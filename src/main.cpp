#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    ofGLFWWindowSettings settings;
    settings.windowMode = OF_WINDOW;
    settings.resizable = false;
    settings.setSize(1024, 768);

    ofCreateWindow(settings);
    ofRunApp(new ofApp);
}
