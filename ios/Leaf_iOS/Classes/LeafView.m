#import "LeafView.h"
#import "LeafRenderer.h"
#import <CoreFoundation/CFString.h>
#import <QuartzCore/CAMetalLayer.h>

@interface LFTextPosition : UITextPosition
@property (nonatomic, assign) NSInteger offset;
+ (instancetype)positionWithOffset:(NSInteger)offset;
@end

@implementation LFTextPosition

+ (instancetype)positionWithOffset:(NSInteger)offset {
    LFTextPosition *position = [[LFTextPosition alloc] init];
    position.offset = offset;
    return position;
}

@end

@interface LFTextRange : UITextRange
@property (nonatomic, strong) LFTextPosition *startPosition;
@property (nonatomic, strong) LFTextPosition *endPosition;
+ (instancetype)rangeWithNSRange:(NSRange)range;
- (NSRange)asNSRange;
@end

@implementation LFTextRange

+ (instancetype)rangeWithNSRange:(NSRange)range {
    LFTextRange *textRange = [[LFTextRange alloc] init];
    textRange.startPosition = [LFTextPosition positionWithOffset:(NSInteger)range.location];
    textRange.endPosition = [LFTextPosition positionWithOffset:(NSInteger)(range.location + range.length)];
    return textRange;
}

- (UITextPosition *)start {
    return self.startPosition;
}

- (UITextPosition *)end {
    return self.endPosition;
}

- (BOOL)isEmpty {
    return self.startPosition.offset == self.endPosition.offset;
}

- (NSRange)asNSRange {
    NSInteger start = MIN(self.startPosition.offset, self.endPosition.offset);
    NSInteger end = MAX(self.startPosition.offset, self.endPosition.offset);
    return NSMakeRange((NSUInteger)start, (NSUInteger)(end - start));
}

@end

@interface LeafView ()
@property (nonatomic, strong) LeafRenderer *renderer;
@property (nonatomic, assign) BOOL textInputFocused;
@property (nonatomic, strong) NSMutableString *textBuffer;
@property (nonatomic, strong) NSMutableString *lastCommittedBuffer;
@property (nonatomic, assign) NSRange selectedRangeValue;
@property (nonatomic, assign) NSRange markedRangeValue;
@property (nonatomic, strong) UITextInputStringTokenizer *textTokenizer;
@end

@implementation LeafView {
    __weak id<UITextInputDelegate> _leafInputDelegate;
    NSDictionary<NSAttributedStringKey, id> *_leafMarkedTextStyle;

    UITextAutocapitalizationType _leafAutocapitalizationType;
    UITextAutocorrectionType _leafAutocorrectionType;
    UITextSpellCheckingType _leafSpellCheckingType;
    UIKeyboardType _leafKeyboardType;
    UIKeyboardAppearance _leafKeyboardAppearance;
    UIReturnKeyType _leafReturnKeyType;
    BOOL _leafSecureTextEntry;
    BOOL _leafEnablesReturnKeyAutomatically;
}

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self leaf_commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (self) {
        [self leaf_commonInit];
    }
    return self;
}

- (void)leaf_commonInit {
    self.contentScaleFactor = UIScreen.mainScreen.scale;
    self.multipleTouchEnabled = YES;
    self.opaque = YES;
    self.textInputFocused = NO;
    self.textBuffer = [NSMutableString string];
    self.lastCommittedBuffer = [NSMutableString string];
    self.selectedRangeValue = NSMakeRange(0, 0);
    self.markedRangeValue = NSMakeRange(NSNotFound, 0);
    self.textTokenizer = [[UITextInputStringTokenizer alloc] initWithTextInput:self];

    self.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.autocorrectionType = UITextAutocorrectionTypeNo;
    self.spellCheckingType = UITextSpellCheckingTypeNo;
    self.keyboardType = UIKeyboardTypeDefault;
    self.keyboardAppearance = UIKeyboardAppearanceDefault;
    self.returnKeyType = UIReturnKeyDone;
    self.secureTextEntry = NO;
    self.enablesReturnKeyAutomatically = NO;

    self.renderer = [[LeafRenderer alloc] initWithView:self];

    __weak typeof(self) weakSelf = self;
    self.renderer.onTextInputFocusChanged = ^(BOOL focused) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
            return;
        }
        [strongSelf handleTextInputFocusChanged:focused];
    };
}

- (void)setOnEngineReady:(dispatch_block_t)onEngineReady {
    _onEngineReady = [onEngineReady copy];
    self.renderer.onEngineReady = _onEngineReady;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self.renderer resizeIfNeeded];
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window) {
        [self startRendering];
        if (self.textInputFocused) {
            [self becomeFirstResponder];
        }
    } else {
        [self stopRendering];
        [self resignFirstResponder];
    }
}

- (void)startRendering {
    [self.renderer start];
}

- (void)stopRendering {
    [self.renderer stop];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.renderer handleTouchesCancelled:touches withEvent:event];
}

#pragma mark - UIResponder / UIKeyInput

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (BOOL)hasText {
    return self.textBuffer.length > 0;
}

- (void)insertText:(NSString *)text {
    NSString *value = text ?: @"";
    if (value.length == 0) {
        return;
    }

    NSRange replaceRange = [self activeReplaceRange];
    [self notifyTextWillChange];
    [self notifySelectionWillChange];
    [self replaceBufferRange:replaceRange withText:value];
    self.markedRangeValue = NSMakeRange(NSNotFound, 0);
    self.selectedRangeValue = NSMakeRange(replaceRange.location + value.length, 0);
    [self notifyTextDidChange];
    [self notifySelectionDidChange];
    [self flushCommittedDeltaIfNeeded];
}

- (void)deleteBackward {
    if ([self hasMarkedTextInternal]) {
        return;
    }

    NSRange deleteRange = [self clampedRange:self.selectedRangeValue];
    if (deleteRange.length == 0) {
        if (deleteRange.location == 0) {
            [self dispatchBackspaceTimes:1];
            return;
        }
        NSRange composed = [self.textBuffer rangeOfComposedCharacterSequenceAtIndex:deleteRange.location - 1];
        deleteRange = composed;
    }

    if (deleteRange.location != NSNotFound && NSMaxRange(deleteRange) <= self.textBuffer.length && deleteRange.length > 0) {
        [self notifyTextWillChange];
        [self notifySelectionWillChange];
        [self.textBuffer deleteCharactersInRange:deleteRange];
        self.selectedRangeValue = NSMakeRange(deleteRange.location, 0);
        [self notifyTextDidChange];
        [self notifySelectionDidChange];
    }

    [self flushCommittedDeltaIfNeeded];
}

#pragma mark - UITextInput core properties

- (id<UITextInputDelegate>)inputDelegate {
    return _leafInputDelegate;
}

- (void)setInputDelegate:(id<UITextInputDelegate>)inputDelegate {
    _leafInputDelegate = inputDelegate;
}

- (NSDictionary<NSAttributedStringKey, id> *)markedTextStyle {
    return _leafMarkedTextStyle;
}

- (void)setMarkedTextStyle:(NSDictionary<NSAttributedStringKey, id> *)markedTextStyle {
    _leafMarkedTextStyle = [markedTextStyle copy];
}

- (UITextAutocapitalizationType)autocapitalizationType {
    return _leafAutocapitalizationType;
}

- (void)setAutocapitalizationType:(UITextAutocapitalizationType)autocapitalizationType {
    _leafAutocapitalizationType = autocapitalizationType;
}

- (UITextAutocorrectionType)autocorrectionType {
    return _leafAutocorrectionType;
}

- (void)setAutocorrectionType:(UITextAutocorrectionType)autocorrectionType {
    _leafAutocorrectionType = autocorrectionType;
}

- (UITextSpellCheckingType)spellCheckingType {
    return _leafSpellCheckingType;
}

- (void)setSpellCheckingType:(UITextSpellCheckingType)spellCheckingType {
    _leafSpellCheckingType = spellCheckingType;
}

- (UIKeyboardType)keyboardType {
    return _leafKeyboardType;
}

- (void)setKeyboardType:(UIKeyboardType)keyboardType {
    _leafKeyboardType = keyboardType;
}

- (UIKeyboardAppearance)keyboardAppearance {
    return _leafKeyboardAppearance;
}

- (void)setKeyboardAppearance:(UIKeyboardAppearance)keyboardAppearance {
    _leafKeyboardAppearance = keyboardAppearance;
}

- (UIReturnKeyType)returnKeyType {
    return _leafReturnKeyType;
}

- (void)setReturnKeyType:(UIReturnKeyType)returnKeyType {
    _leafReturnKeyType = returnKeyType;
}

- (BOOL)isSecureTextEntry {
    return _leafSecureTextEntry;
}

- (void)setSecureTextEntry:(BOOL)secureTextEntry {
    _leafSecureTextEntry = secureTextEntry;
}

- (BOOL)enablesReturnKeyAutomatically {
    return _leafEnablesReturnKeyAutomatically;
}

- (void)setEnablesReturnKeyAutomatically:(BOOL)enablesReturnKeyAutomatically {
    _leafEnablesReturnKeyAutomatically = enablesReturnKeyAutomatically;
}

- (UITextRange *)selectedTextRange {
    return [LFTextRange rangeWithNSRange:[self clampedRange:self.selectedRangeValue]];
}

- (void)setSelectedTextRange:(UITextRange *)selectedTextRange {
    NSRange range = [self nsRangeFromTextRange:selectedTextRange];
    if (range.location == NSNotFound) {
        return;
    }
    [self notifySelectionWillChange];
    self.selectedRangeValue = [self clampedRange:range];
    [self notifySelectionDidChange];
}

- (UITextRange *)markedTextRange {
    if (![self hasMarkedTextInternal]) {
        return nil;
    }
    return [LFTextRange rangeWithNSRange:[self clampedRange:self.markedRangeValue]];
}

- (id<UITextInputTokenizer>)tokenizer {
    return self.textTokenizer;
}

#pragma mark - UITextInput text operations

- (NSString *)textInRange:(UITextRange *)range {
    NSRange nsRange = [self nsRangeFromTextRange:range];
    if (nsRange.location == NSNotFound || NSMaxRange(nsRange) > self.textBuffer.length) {
        return @"";
    }
    return [self.textBuffer substringWithRange:nsRange];
}

- (void)replaceRange:(UITextRange *)range withText:(NSString *)text {
    NSRange nsRange = [self nsRangeFromTextRange:range];
    if (nsRange.location == NSNotFound) {
        return;
    }

    NSString *value = text ?: @"";
    [self notifyTextWillChange];
    [self notifySelectionWillChange];
    [self replaceBufferRange:nsRange withText:value];
    self.markedRangeValue = NSMakeRange(NSNotFound, 0);
    self.selectedRangeValue = NSMakeRange(nsRange.location + value.length, 0);
    [self notifyTextDidChange];
    [self notifySelectionDidChange];
    [self flushCommittedDeltaIfNeeded];
}

- (void)setMarkedText:(NSString *)markedText selectedRange:(NSRange)selectedRange {
    NSString *value = markedText ?: @"";
    NSRange replaceRange = [self activeReplaceRange];

    [self notifyTextWillChange];
    [self notifySelectionWillChange];
    [self replaceBufferRange:replaceRange withText:value];

    if (value.length > 0) {
        NSRange markRange = NSMakeRange(replaceRange.location, value.length);
        self.markedRangeValue = markRange;

        NSUInteger relativeStart = MIN(selectedRange.location, markRange.length);
        NSUInteger remain = markRange.length - relativeStart;
        NSUInteger relativeLength = MIN(selectedRange.length, remain);
        self.selectedRangeValue = NSMakeRange(markRange.location + relativeStart, relativeLength);
    } else {
        self.markedRangeValue = NSMakeRange(NSNotFound, 0);
        self.selectedRangeValue = NSMakeRange(replaceRange.location, 0);
    }

    [self notifyTextDidChange];
    [self notifySelectionDidChange];
}

- (void)unmarkText {
    if (![self hasMarkedTextInternal]) {
        return;
    }

    [self notifySelectionWillChange];
    NSUInteger end = NSMaxRange([self clampedRange:self.markedRangeValue]);
    self.markedRangeValue = NSMakeRange(NSNotFound, 0);
    self.selectedRangeValue = NSMakeRange(end, 0);
    [self notifySelectionDidChange];
    [self flushCommittedDeltaIfNeeded];
}

#pragma mark - UITextInput positions / ranges

- (UITextPosition *)beginningOfDocument {
    return [LFTextPosition positionWithOffset:0];
}

- (UITextPosition *)endOfDocument {
    return [LFTextPosition positionWithOffset:(NSInteger)self.textBuffer.length];
}

- (UITextRange *)textRangeFromPosition:(UITextPosition *)fromPosition toPosition:(UITextPosition *)toPosition {
    NSInteger from = [self offsetForTextPosition:fromPosition];
    NSInteger to = [self offsetForTextPosition:toPosition];
    if (from == NSNotFound || to == NSNotFound) {
        return nil;
    }
    NSInteger start = MIN(from, to);
    NSInteger end = MAX(from, to);
    return [LFTextRange rangeWithNSRange:NSMakeRange((NSUInteger)start, (NSUInteger)(end - start))];
}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position offset:(NSInteger)offset {
    NSInteger current = [self offsetForTextPosition:position];
    if (current == NSNotFound) {
        return nil;
    }
    NSInteger next = [self clampedOffset:(current + offset)];
    return [LFTextPosition positionWithOffset:next];
}

- (UITextPosition *)positionFromPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset {
    NSInteger delta = offset;
    NSInteger absOffset = (offset >= 0) ? offset : -offset;
    switch (direction) {
        case UITextLayoutDirectionLeft:
        case UITextLayoutDirectionUp:
            delta = -absOffset;
            break;
        case UITextLayoutDirectionRight:
        case UITextLayoutDirectionDown:
            delta = absOffset;
            break;
        default:
            break;
    }
    return [self positionFromPosition:position offset:delta];
}

- (NSComparisonResult)comparePosition:(UITextPosition *)position toPosition:(UITextPosition *)other {
    NSInteger a = [self offsetForTextPosition:position];
    NSInteger b = [self offsetForTextPosition:other];
    if (a == NSNotFound || b == NSNotFound) {
        return NSOrderedSame;
    }
    if (a < b) return NSOrderedAscending;
    if (a > b) return NSOrderedDescending;
    return NSOrderedSame;
}

- (NSInteger)offsetFromPosition:(UITextPosition *)from toPosition:(UITextPosition *)toPosition {
    NSInteger a = [self offsetForTextPosition:from];
    NSInteger b = [self offsetForTextPosition:toPosition];
    if (a == NSNotFound || b == NSNotFound) {
        return 0;
    }
    return b - a;
}

- (UITextPosition *)positionWithinRange:(UITextRange *)range farthestInDirection:(UITextLayoutDirection)direction {
    NSRange nsRange = [self nsRangeFromTextRange:range];
    if (nsRange.location == NSNotFound) {
        return nil;
    }

    switch (direction) {
        case UITextLayoutDirectionLeft:
        case UITextLayoutDirectionUp:
            return [LFTextPosition positionWithOffset:(NSInteger)nsRange.location];
        case UITextLayoutDirectionRight:
        case UITextLayoutDirectionDown:
            return [LFTextPosition positionWithOffset:(NSInteger)NSMaxRange(nsRange)];
        default:
            return [LFTextPosition positionWithOffset:(NSInteger)nsRange.location];
    }
}

- (UITextRange *)characterRangeByExtendingPosition:(UITextPosition *)position inDirection:(UITextLayoutDirection)direction {
    NSInteger location = [self offsetForTextPosition:position];
    if (location == NSNotFound) {
        return nil;
    }

    if (self.textBuffer.length == 0) {
        return [LFTextRange rangeWithNSRange:NSMakeRange(0, 0)];
    }

    if (direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp) {
        if (location <= 0) {
            return [LFTextRange rangeWithNSRange:NSMakeRange(0, 0)];
        }
        NSRange range = [self.textBuffer rangeOfComposedCharacterSequenceAtIndex:(NSUInteger)(location - 1)];
        return [LFTextRange rangeWithNSRange:range];
    }

    if (location >= (NSInteger)self.textBuffer.length) {
        return [LFTextRange rangeWithNSRange:NSMakeRange(self.textBuffer.length, 0)];
    }
    NSRange range = [self.textBuffer rangeOfComposedCharacterSequenceAtIndex:(NSUInteger)location];
    return [LFTextRange rangeWithNSRange:range];
}

#pragma mark - UITextInput layout / geometry

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
- (UITextWritingDirection)baseWritingDirectionForPosition:(UITextPosition *)position inDirection:(UITextStorageDirection)direction {
    (void)position;
    (void)direction;
    return UITextWritingDirectionNatural;
}

- (void)setBaseWritingDirection:(UITextWritingDirection)writingDirection forRange:(UITextRange *)range {
    (void)writingDirection;
    (void)range;
}
#pragma clang diagnostic pop

- (CGRect)firstRectForRange:(UITextRange *)range {
    NSRange nsRange = [self nsRangeFromTextRange:range];
    if (nsRange.location == NSNotFound) {
        return CGRectZero;
    }
    UITextPosition *position = [LFTextPosition positionWithOffset:(NSInteger)nsRange.location];
    return [self caretRectForPosition:position];
}

- (CGRect)caretRectForPosition:(UITextPosition *)position {
    NSInteger offset = [self offsetForTextPosition:position];
    if (offset == NSNotFound) {
        return CGRectZero;
    }

    const CGFloat inset = 8.0f;
    const CGFloat glyphWidth = 8.0f;
    const CGFloat caretWidth = 2.0f;
    const CGFloat caretHeight = 24.0f;
    CGFloat x = inset + glyphWidth * (CGFloat)offset;
    CGFloat maxX = MAX(inset, self.bounds.size.width - inset - caretWidth);
    if (x > maxX) {
        x = maxX;
    }
    CGFloat y = MAX(inset, self.bounds.size.height - inset - caretHeight);
    return CGRectMake(x, y, caretWidth, caretHeight);
}

- (NSArray<UITextSelectionRect *> *)selectionRectsForRange:(UITextRange *)range {
    (void)range;
    return @[];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point {
    const CGFloat inset = 8.0f;
    const CGFloat glyphWidth = 8.0f;
    CGFloat raw = (point.x - inset) / glyphWidth;
    NSInteger offset = (NSInteger)llround(raw);
    return [LFTextPosition positionWithOffset:[self clampedOffset:offset]];
}

- (UITextPosition *)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange *)range {
    NSRange nsRange = [self nsRangeFromTextRange:range];
    if (nsRange.location == NSNotFound) {
        return [self closestPositionToPoint:point];
    }
    LFTextPosition *position = (LFTextPosition *)[self closestPositionToPoint:point];
    NSInteger location = [self clampedOffset:position.offset];
    NSInteger minLocation = (NSInteger)nsRange.location;
    NSInteger maxLocation = (NSInteger)NSMaxRange(nsRange);
    if (location < minLocation) {
        location = minLocation;
    } else if (location > maxLocation) {
        location = maxLocation;
    }
    return [LFTextPosition positionWithOffset:location];
}

- (UITextRange *)characterRangeAtPoint:(CGPoint)point {
    LFTextPosition *position = (LFTextPosition *)[self closestPositionToPoint:point];
    return [self characterRangeByExtendingPosition:position inDirection:UITextLayoutDirectionRight];
}

#pragma mark - UITextInput optional edit checks

- (BOOL)shouldChangeTextInRange:(UITextRange *)range replacementText:(NSString *)text {
    (void)range;
    (void)text;
    return YES;
}

#pragma mark - Hardware keyboard support

- (BOOL)dispatchSpecialKeyPresses:(NSSet<UIPress *> *)presses isDown:(BOOL)isDown {
    BOOL supportsHardwareKeyAPI = NO;
    if (@available(iOS 13.4, *)) {
        supportsHardwareKeyAPI = YES;
    }
    if (!supportsHardwareKeyAPI) {
        return NO;
    }

    BOOL handled = NO;
    for (UIPress *press in presses) {
        UIKey *key = press.key;
        if (!key) {
            continue;
        }

        int keyCode = [self.renderer mapIOSHidKeyCode:key.keyCode];
        if (keyCode == 0) {
            keyCode = [self.renderer mapIOSInputKey:key.charactersIgnoringModifiers];
        }
        if (keyCode == 0) {
            continue;
        }

        BOOL special = (
            keyCode == 9 ||
            keyCode == 13 ||
            keyCode == 27 ||
            keyCode == 127 ||
            keyCode == 1001 ||
            keyCode == 1002 ||
            keyCode == 1003 ||
            keyCode == 1004 ||
            keyCode == 1005 ||
            keyCode == 1006
        );
        if (!special) {
            continue;
        }

        uint32_t modifiers = [self.renderer mapIOSModifierFlags:key.modifierFlags];
        [self.renderer queueKeyEventWithType:isDown ? [self.renderer keyEventTypeDown] : [self.renderer keyEventTypeUp]
                                     keyCode:keyCode
                                   modifiers:modifiers
                                      repeat:NO];
        handled = YES;
    }
    return handled;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    if (![self dispatchSpecialKeyPresses:presses isDown:YES]) {
        [super pressesBegan:presses withEvent:event];
    }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    if (![self dispatchSpecialKeyPresses:presses isDown:NO]) {
        [super pressesEnded:presses withEvent:event];
    }
}

- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self dispatchSpecialKeyPresses:presses isDown:NO];
    [super pressesCancelled:presses withEvent:event];
}

#pragma mark - Focus sync

- (void)handleTextInputFocusChanged:(BOOL)focused {
    if (self.textInputFocused == focused) {
        return;
    }
    self.textInputFocused = focused;

    if (focused) {
        if (self.window) {
            [self becomeFirstResponder];
        }
    } else {
        [self resignFirstResponder];
        [self notifySelectionWillChange];
        self.markedRangeValue = NSMakeRange(NSNotFound, 0);
        [self.textBuffer setString:@""];
        [self.lastCommittedBuffer setString:@""];
        self.selectedRangeValue = NSMakeRange(0, 0);
        [self notifySelectionDidChange];
    }
}

#pragma mark - Session helpers

- (NSInteger)clampedOffset:(NSInteger)offset {
    if (offset < 0) {
        return 0;
    }
    NSInteger maxOffset = (NSInteger)self.textBuffer.length;
    if (offset > maxOffset) {
        return maxOffset;
    }
    return offset;
}

- (NSRange)clampedRange:(NSRange)range {
    if (range.location == NSNotFound) {
        return NSMakeRange(NSNotFound, 0);
    }

    NSUInteger maxLen = self.textBuffer.length;
    NSUInteger location = MIN(range.location, maxLen);
    NSUInteger remain = maxLen - location;
    NSUInteger length = MIN(range.length, remain);
    return NSMakeRange(location, length);
}

- (NSInteger)offsetForTextPosition:(UITextPosition *)position {
    if (![position isKindOfClass:[LFTextPosition class]]) {
        return NSNotFound;
    }
    LFTextPosition *textPosition = (LFTextPosition *)position;
    return [self clampedOffset:textPosition.offset];
}

- (NSRange)nsRangeFromTextRange:(UITextRange *)range {
    if (![range isKindOfClass:[LFTextRange class]]) {
        return NSMakeRange(NSNotFound, 0);
    }
    LFTextRange *textRange = (LFTextRange *)range;
    return [self clampedRange:[textRange asNSRange]];
}

- (BOOL)hasMarkedTextInternal {
    return self.markedRangeValue.location != NSNotFound &&
           NSMaxRange([self clampedRange:self.markedRangeValue]) <= self.textBuffer.length;
}

- (NSRange)activeReplaceRange {
    if ([self hasMarkedTextInternal]) {
        return [self clampedRange:self.markedRangeValue];
    }
    return [self clampedRange:self.selectedRangeValue];
}

- (void)replaceBufferRange:(NSRange)range withText:(NSString *)text {
    NSRange safeRange = [self clampedRange:range];
    if (safeRange.location == NSNotFound) {
        return;
    }
    [self.textBuffer replaceCharactersInRange:safeRange withString:(text ?: @"")];
}

- (NSUInteger)graphemeCountInString:(NSString *)text {
    if (text.length == 0) {
        return 0;
    }
    __block NSUInteger count = 0;
    [text enumerateSubstringsInRange:NSMakeRange(0, text.length)
                             options:NSStringEnumerationByComposedCharacterSequences
                          usingBlock:^(__unused NSString *substring, __unused NSRange substringRange, __unused NSRange enclosingRange, __unused BOOL *stop) {
        count += 1;
    }];
    return count;
}

- (void)dispatchBackspaceTimes:(NSUInteger)count {
    int keyCode = [self.renderer mapIOSInputKey:@"\b"];
    NSUInteger loops = MAX((NSUInteger)1, count);
    for (NSUInteger i = 0; i < loops; i++) {
        [self.renderer queueKeyEventWithType:[self.renderer keyEventTypeDown]
                                     keyCode:keyCode
                                   modifiers:0
                                      repeat:NO];
        [self.renderer queueKeyEventWithType:[self.renderer keyEventTypeUp]
                                     keyCode:keyCode
                                   modifiers:0
                                      repeat:NO];
    }
}

- (void)queueCommittedText:(NSString *)text {
    if (text.length == 0) {
        return;
    }

    NSUInteger index = 0;
    while (index < text.length) {
        UTF32Char codepoint = 0;
        unichar high = [text characterAtIndex:index];
        if (CFStringIsSurrogateHighCharacter(high) && index + 1 < text.length) {
            unichar low = [text characterAtIndex:index + 1];
            if (CFStringIsSurrogateLowCharacter(low)) {
                codepoint = CFStringGetLongCharacterForSurrogatePair(high, low);
                index += 2;
            } else {
                codepoint = high;
                index += 1;
            }
        } else {
            codepoint = high;
            index += 1;
        }

        if (codepoint == '\n' || codepoint == '\r') {
            int keyCode = [self.renderer mapIOSInputKey:@"\n"];
            [self.renderer queueKeyEventWithType:[self.renderer keyEventTypeDown]
                                         keyCode:keyCode
                                       modifiers:0
                                          repeat:NO];
            [self.renderer queueKeyEventWithType:[self.renderer keyEventTypeUp]
                                         keyCode:keyCode
                                       modifiers:0
                                          repeat:NO];
            continue;
        }

        if (codepoint > 0) {
            [self.renderer queueCharInput:(uint32_t)codepoint];
        }
    }
}

- (void)flushCommittedDeltaIfNeeded {
    if ([self hasMarkedTextInternal]) {
        return;
    }

    NSString *current = [self.textBuffer copy];
    NSString *committed = [self.lastCommittedBuffer copy];
    if ([current isEqualToString:committed]) {
        return;
    }

    NSUInteger currentLength = current.length;
    NSUInteger committedLength = committed.length;

    NSUInteger prefix = 0;
    NSUInteger prefixLimit = MIN(currentLength, committedLength);
    while (prefix < prefixLimit &&
           [current characterAtIndex:prefix] == [committed characterAtIndex:prefix]) {
        prefix += 1;
    }

    NSUInteger suffix = 0;
    while (suffix + prefix < currentLength &&
           suffix + prefix < committedLength &&
           [current characterAtIndex:(currentLength - 1 - suffix)] ==
           [committed characterAtIndex:(committedLength - 1 - suffix)]) {
        suffix += 1;
    }

    NSUInteger deleteLength = committedLength - prefix - suffix;
    NSUInteger insertLength = currentLength - prefix - suffix;

    if (deleteLength > 0) {
        NSString *deleted = [committed substringWithRange:NSMakeRange(prefix, deleteLength)];
        NSUInteger backspaceCount = [self graphemeCountInString:deleted];
        if (backspaceCount > 0) {
            [self dispatchBackspaceTimes:backspaceCount];
        }
    }

    if (insertLength > 0) {
        NSString *inserted = [current substringWithRange:NSMakeRange(prefix, insertLength)];
        [self queueCommittedText:inserted];
    }

    [self.lastCommittedBuffer setString:current];
}

- (void)notifyTextWillChange {
    id<UITextInputDelegate> delegate = self.inputDelegate;
    if (delegate) {
        [delegate textWillChange:self];
    }
}

- (void)notifyTextDidChange {
    id<UITextInputDelegate> delegate = self.inputDelegate;
    if (delegate) {
        [delegate textDidChange:self];
    }
}

- (void)notifySelectionWillChange {
    id<UITextInputDelegate> delegate = self.inputDelegate;
    if (delegate) {
        [delegate selectionWillChange:self];
    }
}

- (void)notifySelectionDidChange {
    id<UITextInputDelegate> delegate = self.inputDelegate;
    if (delegate) {
        [delegate selectionDidChange:self];
    }
}

@end
