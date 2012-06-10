import QtQuick 1.0
import Mappero 1.0
import com.nokia.meego 1.0

PageStackWindow {
    initialPage: mainPage
    showToolBar: false
    visible: platformWindow.visible

    Page {
        id: mainPage

        MainPage {
            // HACK: For some reason, the __isPage property defined by Page is
            // not visible here.
            property bool __isPage: true
        }
    }
}
