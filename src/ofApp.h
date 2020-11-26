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
    void changedCompMode(bool &mode);
    void changedDevice(int &newDevice);
    void changedSampleRate(int &rate);
    void changedBufferSize(int &size);
    void changedOffset(float &offset);
    void wasReset(bool &isReset);
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
    ofParameter<int> in1Channel;
    ofParameter<int> in2Channel;
    ofParameter<float> ch1Gain;
    ofParameter<float> ch2Gain;
    ofParameter<float> timeWidth;
    ofParameter<float> offset;
    ofParameter<bool> compMode;
    ofParameter<float> y_max;
    ofParameter<float> y_min;
    ofParameter<bool> freeze;
    ofParameter<bool> reset;

    int audioDeviceDefault;
    int sampleRateDefault;
    int bufferSizeDefault;
    int in1ChannelDefault;
    int in2ChannelDefault;
    float ch1GainDefault;
    float ch2GainDefault;
    float timeWidthDefault;
    float offsetDefault;
    bool compModeDefault;
    float y_maxDefault;
    float y_minDefault;
    bool freezeDefault;

    vector <float> input1;
    vector <float> input2;
    vector <std::pair<float,float>> volHistory;
    unsigned long sampleCount = 0;
    bool setInputs = false;

    ofSoundStream soundStream;
    ofSoundStreamSettings settings;
    ofSoundDevice device;

    ofColor black = ofColor(0,0,0);
    ofColor white = ofColor(255,255,255);
    ofColor lightgray = ofColor(225,225,225);
    ofColor gray = ofColor(127,127,127);
    ofColor darkgray = ofColor(57,57,57);

    ofColor red = ofColor(225,127,127);
    ofColor blue = ofColor(127,127,225);
    ofColor green = ofColor(87,185,87);

    int plotHeight;
    int plotWidth;
};
