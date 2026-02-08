import { LeafView } from 'leaf_ohos'

@Entry
@Component
struct Index {

  build() {
    RelativeContainer() {
      LeafView()
        .height('100%')
        .width('100%')
    }
    .height('100%')
    .width('100%')
  }
}