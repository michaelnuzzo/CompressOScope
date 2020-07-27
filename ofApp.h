#pragma once

#include "ofMain.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

public:

    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    void audioIn(ofSoundBuffer & input);
    void changedMode(bool &mode);
    void changedDevice(int &newDevice);
    void changedSampleRate(int &rate);
    void changedBufferSize(int &rate);
    void froze(bool &frozen);
    void exit();

private:

    ofxPanel guiLeft;
    ofxPanel guiRight;
    ofParameterGroup audioInterface;
    ofParameterGroup channels;
    ofParameterGroup mainControls;
    ofParameterGroup compressionControls;
    ofParameterGroup collectorLeft;
    ofParameterGroup collectorRight;

    ofParameter<int> audioDevice;
    ofParameter<int> sampleRate;
    ofParameter<int> bufferSize;
    ofParameter<int> input1Channel;
    ofParameter<int> input2Channel;
    ofParameter<float> timeWidth;
    ofParameter<bool> compressionMode;
    ofParameter<float> y_max;
    ofParameter<float> y_min;
    ofParameter<bool> freeze;

    vector <float> input1;
    vector <float> input2;
    vector <std::pair<float,float> > volHistory;

    ofSoundStream soundStream;
    ofSoundStreamSettings settings;
    ofSoundDevice device;

    int plotHeight;
    int plotWidth;
};
