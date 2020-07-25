 #include "ofApp.h"

// find max and min value in passed in audio chunk
std::vector<float> findMaxAndMin(std::vector<float> audioFrame,int numSelections)
{
    std::vector<float> selections;
    for(int i = 0; i < numSelections; i++)
        if(i%2==0)
            selections.push_back(*max_element(std::begin(audioFrame), std::end(audioFrame)));
        else
            selections.push_back(*min_element(std::begin(audioFrame), std::end(audioFrame)));
    return selections;
}
// format the axis numbers
std::string format_number(float num)
{
    std::stringstream stream;
    stream << std::right << std::setw(6) << std::fixed << std::setprecision(2) << num;
    return stream.str();
}
// convert to decibels
float toDb(float num)
{
    return 20*log10(abs(num));
}
// convert from decibels
float toMag(float num)
{
    return pow(10,num/20);
}

// listener
void ofApp::changedMode(bool &mode)
{
    volHistory.clear();
}
// listener
void ofApp::changedDevice(int &newDevice)
{
    soundStream.stop();
    device = soundStream.getDeviceList().at(newDevice);
    settings.setInDevice(device);
    settings.setInListener(this);
    settings.numOutputChannels = 0;
    settings.numInputChannels = device.inputChannels;
    soundStream.setup(settings);
    input1Channel.setMax(device.inputChannels);
    input2Channel.setMax(device.inputChannels);
    sampleRate.setMax(device.sampleRates.size()-1);
    input1Channel = 1;
    input2Channel = 2;
    sampleRate = 0;
    volHistory.clear();
}
// listener
void ofApp::changedSampleRate(int &rate)
{
    settings.sampleRate = device.sampleRates.at(rate);
    soundStream.stop();
    soundStream.setup(settings);
    volHistory.clear();
}
// listener
void ofApp::changedBufferSize(int &size)
{
    int val = pow(2.,float(size));
    settings.bufferSize = val;
    soundStream.stop();
    soundStream.setup(settings);
    volHistory.clear();
}
// listener
void ofApp::froze(bool &frozen)
{
    if(!frozen)
        volHistory.clear();
}

// this saves the gui state upon quitting the app
void ofApp::exit() {
    gui.saveToFile("settings-2.json");
}

//--------------------------------------------------------------
// this sets up the application
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofBackground(54, 54, 54);

    // to store data within the .app
    ofSetDataPathRoot("../Resources/data/");

    // custom plot dimensions
    plotHeight = ofGetHeight()/2.;
    plotWidth = ofGetWidth()*0.93;

    volHistory.clear();

    int deviceidx;
    for(int i = 0; i < soundStream.getDeviceList().size(); i++)
        if(soundStream.getDeviceList().at(i).isDefaultInput)
        {
            device = soundStream.getDeviceList().at(i);
            deviceidx = i;
        }
    settings.setInDevice(device);

    // set up the gui and add event listeners
    gui.setup("Controls","settings.json");
    gui.add(audioDevice.setup("Audio Device Selector",deviceidx,0,soundStream.getDeviceList().size()-1));
    audioDevice.addListener(this, &ofApp::changedDevice);
    audioDevice.setUpdateOnReleaseOnly(true);
    gui.add(sampleRate.setup("Sample Rate Selector",0,0,device.sampleRates.size()-1));
    sampleRate.addListener(this, &ofApp::changedSampleRate);
    sampleRate.setUpdateOnReleaseOnly(true);
    gui.add(bufferSize.setup("Buffer Size",6,6,11));
    bufferSize.addListener(this, &ofApp::changedBufferSize);
    bufferSize.setUpdateOnReleaseOnly(true);
    gui.add(timeWidth.setup("time (s)",2.5,0.01,5));
    gui.add(compressionMode.setup("Compression Analysis",false));
    compressionMode.addListener(this, &ofApp::changedMode);
    gui.add(y_max.setup("Y max (dB)",0,-200,20));
    gui.add(y_min.setup("Y min (dB)",-60,-200,20));
    gui.add(input1Channel.setup("Reference Channel",1,1,device.inputChannels));
    gui.add(input2Channel.setup("Divisor Channel",2,1,device.inputChannels));
    gui.add(freeze.setup("Freeze",false));
    freeze.addListener(this, &ofApp::froze);


    // set up the initial settings of the audio device
    settings.setInListener(this);
    settings.numOutputChannels = 0;
    settings.numInputChannels = device.inputChannels;
    soundStream.setup(settings);

    gui.loadFromFile("settings-2.json");

}

//--------------------------------------------------------------
// this is the real-time graphics loop
void ofApp::update(){
    //if we are bigger the the size we want to record - lets drop the oldest value
    while( volHistory.size() >= plotWidth)
        volHistory.erase(volHistory.begin(), volHistory.begin()+1);
}

//--------------------------------------------------------------
// this is (also) the real-time graphics loop
void ofApp::draw(){

    ofSetColor(225);
    ofNoFill();

    // draw the plot
    ofPushStyle();
        ofPushMatrix();
            ofTranslate(ofGetWidth()-plotWidth-ofGetWidth()/200., 1.2*ofGetHeight()/2.-plotHeight/2.);
            ofDrawRectangle(0, 0, plotWidth, plotHeight);
            // draw the waveform
            ofBeginShape();
                for (unsigned int i = 0; i < volHistory.size(); i++)
                {
                    if(compressionMode)
                        ofVertex(i, ofMap(toDb(volHistory[i]),-y_max,-y_min,0,plotHeight,true)); // compressor gain
                    else
                        ofVertex(i, ofClamp(plotHeight/2.-volHistory[i]*plotHeight,0,plotHeight));

                }
            ofEndShape(false);
            // x-axis
            ofDrawBitmapString("Time (s)", plotWidth/2.-20, plotHeight+48);
            ofDrawBitmapString("0", plotWidth-8, plotHeight+20);
            ofDrawBitmapString(format_number(timeWidth/4.), 3*plotWidth/4.-35, plotHeight+20);
            ofDrawBitmapString(format_number(timeWidth/2.), plotWidth/2.-35, plotHeight+20);
            ofDrawBitmapString(format_number(3*timeWidth/4.), plotWidth/4.-35, plotHeight+20);
            ofDrawBitmapString(format_number(timeWidth), -35, plotHeight+20);

            // y-axis
            if(compressionMode)
            {
                ofDrawBitmapString("Gain (dB)", -15, -15);
                ofDrawBitmapString(format_number(y_max), -60, 5);
                ofDrawBitmapString(format_number(2*y_max/3.+y_min/3.), -60, plotHeight/3.+5);
                ofDrawBitmapString(format_number(y_max/3.+2*y_min/3.), -60, 2*plotHeight/3.+5);
                ofDrawBitmapString(format_number(y_min), -60, plotHeight+5);
            }
            else
            {
                ofDrawBitmapString("Magnitude", -15, -15);
                ofDrawBitmapString(format_number(1), -60, 5);
                ofDrawBitmapString(format_number(0.5), -60, plotHeight/4.+5);
                ofDrawBitmapString(format_number(0), -60, plotHeight/2.+5);
                ofDrawBitmapString(format_number(-0.5), -60, 3*plotHeight/4.+5);
                ofDrawBitmapString(format_number(-1), -60, plotHeight+5);
            }

        ofPopMatrix();
    ofPopStyle();
    gui.draw();
    ofDrawBitmapString(soundStream.getDeviceList().at(audioDevice).name, 220, 44);
    ofDrawBitmapString(device.sampleRates.at(sampleRate), 220, 64);
    ofDrawBitmapString(to_string(int(pow(2.,float(bufferSize)))), 220, 84);


}

//--------------------------------------------------------------
// this is the real-time audio loop
void ofApp::audioIn(ofSoundBuffer & input){
    if(!freeze)
        for (size_t i = 0; i < input.getNumFrames(); i++)
        {
            input1.push_back(input[i*device.inputChannels+(input1Channel-1)]);
            if(compressionMode)
                input2.push_back(input[i*device.inputChannels+input2Channel-1]);

            // fill according to time window specification
            if(input1.size() > 2*timeWidth*input.getSampleRate()/ofGetWidth())
            {
                // need to find max and min vals to display zoomed-out info
                std::vector<float> selections1 = findMaxAndMin(input1,2);
                std::vector<float> selections2;
                if(compressionMode)
                    selections2 = findMaxAndMin(input2,2);
                // record the max and min info
                for(int j = 0; j < selections1.size(); j++)
                    if(compressionMode)
                        volHistory.push_back(selections1[j]/selections2[j]);
                    else
                        volHistory.push_back(selections1[j]);

                input1.clear();
                if(compressionMode)
                    input2.clear();
            }
        }
}



//--------------------------------------------------------------
void ofApp::keyPressed  (int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

