#include "../cpp/LFFilePicker.h"

#include <cctype>
#include <cstdio>

namespace {

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string runCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    pclose(pipe);
    return trim(output);
}

}

bool lfFilePickerRequestFromPlatform(int requestId, const LFFilePickerOptions& options, std::string& error) {
    (void) error;

    std::string command = "osascript -e 'try' ";
    switch (options.mediaType) {
        case LFFilePickerMediaType::Image:
            command += "-e 'POSIX path of (choose file with prompt \"Select an image\" of type {\"public.image\"})' ";
            break;
        case LFFilePickerMediaType::Video:
            command += "-e 'POSIX path of (choose file with prompt \"Select a video\" of type {\"public.movie\"})' ";
            break;
        case LFFilePickerMediaType::ImageOrVideo:
            command += "-e 'POSIX path of (choose file with prompt \"Select a media\" of type {\"public.image\", \"public.movie\"})' ";
            break;
        case LFFilePickerMediaType::Any:
        default:
            command += "-e 'POSIX path of (choose file with prompt \"Select a file\")' ";
            break;
    }
    command += "-e 'on error number -128' ";
    command += "-e 'return \"\"' ";
    command += "-e 'end try'";

    const std::string path = runCommand(command);

    if (path.empty()) {
        lfFilePickerOnPlatformResult(requestId, 0, "", "canceled");
    } else {
        lfFilePickerOnPlatformResult(requestId, 1, path.c_str(), "");
    }
    return true;
}
