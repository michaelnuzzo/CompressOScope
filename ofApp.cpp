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
    if(mode)
    {
        guiLeft.getGroup("Channels").getIntSlider("Reference Channel").setBackgroundColor(ofColor(0,0,0));
        guiLeft.getGroup("Channels").getIntSlider("Divisor Channel").setBackgroundColor(ofColor(0,0,0));
    }
    else
    {
        guiLeft.getGroup("Channels").getIntSlider("Reference Channel").setBackgroundColor(ofColor(255,127,127));
        guiLeft.getGroup("Channels").getIntSlider("Divisor Channel").setBackgroundColor(ofColor(127,127,255));
    }

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
void ofApp::exit()
{
    guiLeft.saveToFile("settings-autosave-left.xml");
    guiRight.saveToFile("settings-autosave-right.xml");
}

//--------------------------------------------------------------
// this sets up the application
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofBackground(54, 54, 54);

    // to store data within the .app
    ofSetDataPathRoot(".");
    if(!ofDirectory::doesDirectoryExist("gui-settings"))
        ofDirectory::createDirectory("gui-settings");
    ofSetDataPathRoot("gui-settings");

    if(!ofFile::doesFileExist("settings-manualsave-left.xml"))
        guiLeft.saveToFile("settings-manualsave-left.xml");
    if(!ofFile::doesFileExist("settings-autosave-left.xml"))
        guiLeft.saveToFile("settings-autosave-left.xml");
    if(!ofFile::doesFileExist("settings-manualsave-right.xml"))
        guiRight.saveToFile("settings-manualsave-right.xml");
    if(!ofFile::doesFileExist("settings-autosave-right.xml"))
        guiRight.saveToFile("settings-autosave-right.xml");

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
    audioInterface.add(audioDevice.set("Audio Device Selector",deviceidx,0,soundStream.getDeviceList().size()-1));
    audioDevice.addListener(this, &ofApp::changedDevice);
    audioInterface.add(sampleRate.set("Sample Rate Selector",0,0,device.sampleRates.size()-1));
    sampleRate.addListener(this, &ofApp::changedSampleRate);
    audioInterface.add(bufferSize.set("Buffer Size",6,6,11));
    bufferSize.addListener(this, &ofApp::changedBufferSize);
    channels.add(input1Channel.set("Reference Channel",1,1,device.inputChannels));
    channels.add(input2Channel.set("Divisor Channel",2,1,device.inputChannels));
    mainControls.add(timeWidth.set("time (s)",2.5,0.01,5));
    compressionControls.add(compressionMode.set("Compression Analysis",false));
    compressionMode.addListener(this, &ofApp::changedMode);
    compressionControls.add(y_max.set("Y max (dB)",0,-200,20));
    compressionControls.add(y_min.set("Y min (dB)",-60,-200,20));
    mainControls.add(freeze.set("Freeze",false));
    freeze.addListener(this, &ofApp::froze);

    audioInterface.setName("Audio Interface");
    collectorLeft.add(audioInterface);
    channels.setName("Channels");
    collectorLeft.add(channels);
    mainControls.setName("Main");
    collectorRight.add(mainControls);
    compressionControls.setName("Compression Mode");
    collectorRight.add(compressionControls);


    guiLeft.setup(collectorLeft,"settings-manualsave-left.xml");
    guiRight.setup(collectorRight,"settings-manualsave-right.xml");
    guiRight.setPosition(ofGetWidth()-guiRight.getWidth()-10,10);

    guiLeft.getGroup("Audio Interface").getIntSlider("Audio Device Selector").setUpdateOnReleaseOnly(true);
    guiLeft.getGroup("Audio Interface").getIntSlider("Sample Rate Selector").setUpdateOnReleaseOnly(true);
    guiLeft.getGroup("Audio Interface").getIntSlider("Buffer Size").setUpdateOnReleaseOnly(true);
    guiLeft.getGroup("Channels").getIntSlider("Reference Channel").setBackgroundColor(ofColor(255,127,127));
    guiLeft.getGroup("Channels").getIntSlider("Divisor Channel").setBackgroundColor(ofColor(127,127,255));


    // set up the initial settings of the audio device
    settings.setInListener(this);
    settings.numOutputChannels = 0;
    settings.numInputChannels = device.inputChannels;
    soundStream.setup(settings);

    guiLeft.loadFromFile("settings-autosave-left.xml");
    guiRight.loadFromFile("settings-autosave-right.xml");

}

//--------------------------------------------------------------
// this is the real-time graphics loop
void ofApp::update(){
}

//--------------------------------------------------------------
// this is (also) the real-time graphics loop
void ofApp::draw(){

    ofSetColor(225);
    ofNoFill();

    // draw the plot
    ofPushMatrix();
        ofTranslate(ofGetWidth()-plotWidth-ofGetWidth()/200., 1.2*ofGetHeight()/2.-plotHeight/2.);
        ofDrawRectangle(0, 0, plotWidth, plotHeight);
        // draw the waveform
            if(compressionMode)
            {
                ofBeginShape();
                for (unsigned int i = 0; i < volHistory.size(); i++)
                        ofVertex(i, ofMap(toDb(volHistory[i].first/volHistory[i].second),-y_max,-y_min,0,plotHeight,true)); // compressor gain
                ofEndShape(false);
            }
            else
            {
                ofPushStyle();
                    ofSetColor(225,127,127);
                    ofBeginShape();
                    for (unsigned int i = 0; i < volHistory.size(); i++)
                        ofVertex(i, ofClamp(plotHeight/2.-volHistory[i].first*plotHeight,0,plotHeight));
                    ofEndShape(false);

                    ofSetColor(127,127,225);
                    ofBeginShape();
                    for (unsigned int i = 0; i < volHistory.size(); i++)
                        ofVertex(i, ofClamp(plotHeight/2.-volHistory[i].second*plotHeight,0,plotHeight));
                    ofEndShape(false);
                ofPopStyle();
            }

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

    if(!guiLeft.getGroup("Audio Interface").isMinimized())
    {
        ofDrawBitmapString(soundStream.getDeviceList().at(audioDevice).name, guiLeft.getPosition().x+210, guiLeft.getPosition().y+57);
        ofDrawBitmapString(device.sampleRates.at(sampleRate), guiLeft.getPosition().x+210, guiLeft.getPosition().y+76);
        ofDrawBitmapString(to_string(int(pow(2.,float(bufferSize)))), guiLeft.getPosition().x+210, guiLeft.getPosition().y+95);
    }
    guiLeft.draw();
    guiRight.draw();
}

//--------------------------------------------------------------
// this is the real-time audio loop
void ofApp::audioIn(ofSoundBuffer & input){
    if(!freeze)
        for (size_t i = 0; i < input.getNumFrames(); i++)
        {
            input1.push_back(input[i*device.inputChannels+(input1Channel-1)]);
            input2.push_back(input[i*device.inputChannels+input2Channel-1]);

            // fill according to time window specification
            if(input1.size() > 2*timeWidth*input.getSampleRate()/ofGetWidth())
            {
                // need to find max and min vals to display zoomed-out info
                std::vector<float> selections1 = findMaxAndMin(input1,2);
                std::vector<float> selections2 = findMaxAndMin(input2,2);
                // record the max and min info
                for(int j = 0; j < 2; j++)
                    volHistory.push_back(std::make_pair(selections1[j],selections2[j]));

                //if we are bigger the the size we want to record - lets drop the oldest value
                if( volHistory.size() >= plotWidth)
                    volHistory.erase(volHistory.begin(), volHistory.begin()+2);

                input1.clear();
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

