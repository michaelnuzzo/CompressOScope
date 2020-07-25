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
        settings.numInputChannels = 2;
    else
        settings.numInputChannels = 1;
    soundStream.setup(settings);
    volHistory.clear();
}
// listener
void ofApp::changedSampleRate(float &rate)
{
    settings.sampleRate = int(rate*1000);
    soundStream.setup(settings);
    volHistory.clear();
}
// listener
void ofApp::changedBufferSize(int &size)
{
    settings.bufferSize = pow(2.,float(size));
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
    gui.saveToFile( "settings.json" );
}

//--------------------------------------------------------------
// this sets up the application
void ofApp::setup(){
    ofSetVerticalSync(true);
    ofBackground(54, 54, 54);

    // to store data within the .app
    ofSetDataPathRoot("../Resources/data/");

    // set up the gui and add event listeners
    gui.setup("Controls","settings.json");
    gui.add(sampleRate.setup("Sample Rate (kHz)",44.1,0.0,1000.0));
    sampleRate.addListener(this, &ofApp::changedSampleRate);
    gui.add(bufferSize.setup("Buffer Size (2^n)",6,6,11));
    bufferSize.addListener(this, &ofApp::changedBufferSize);
    gui.add(timeWidth.setup("time (s)",2.5,0.01,5));
    gui.add(compressionMode.setup("Compression Analysis",false));
    compressionMode.addListener(this, &ofApp::changedMode);
    gui.add(y_max.setup("Y max (dB)",0,-200,20));
    gui.add(y_min.setup("Y min (dB)",-60,-200,20));
    gui.add(freeze.setup("Freeze",false));
    freeze.addListener(this, &ofApp::froze);
    gui.loadFromFile("settings.json");

    // custom plot dimensions
    plotHeight = ofGetHeight()/2.;
    plotWidth = ofGetWidth()*0.93;

    volHistory.clear();

    // set the input audio device to the one currently set as default
    auto devices = soundStream.getMatchingDevices("default");
    if(!devices.empty()){
        settings.setInDevice(devices[0]);
    }

    // set up the initial settings of the audio device
    settings.setInListener(this);
    settings.sampleRate = int(sampleRate*1000);
    settings.numOutputChannels = 0;
    if(compressionMode)
        settings.numInputChannels = 2;
    else
        settings.numInputChannels = 1;
    settings.bufferSize = int(pow(2.,float(bufferSize)));
    soundStream.setup(settings);
}

//--------------------------------------------------------------
// this is the real-time graphics loop
void ofApp::update(){
    //if we are bigger the the size we want to record - lets drop the oldest value
    while( volHistory.size() >= plotWidth){
        volHistory.erase(volHistory.begin(), volHistory.begin()+1);
    }
}

//--------------------------------------------------------------
// this is (also) the real-time graphics loop
void ofApp::draw(){

    ofSetColor(225);
    ofNoFill();

    // draw the plot
    ofPushStyle();
        ofPushMatrix();
            ofTranslate(ofGetWidth()-plotWidth-ofGetWidth()/200., 1.1*ofGetHeight()/2.-plotHeight/2.);
            ofDrawRectangle(0, 0, plotWidth, plotHeight);
            // draw the waveform
            ofBeginShape();
                for (unsigned int i = 0; i < volHistory.size(); i++)
                {
                    if(soundStream.getNumInputChannels() == 1)
                        ofVertex(i, ofClamp(plotHeight/2.-volHistory[i]*plotHeight,0,plotHeight));
                    else if(soundStream.getNumInputChannels() == 2)
                        ofVertex(i, ofMap(toDb(volHistory[i]),-y_max,-y_min,0,plotHeight,true)); // compressor gain
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
            if(soundStream.getNumInputChannels() == 1)
            {
                ofDrawBitmapString("Magnitude", -15, -15);
                ofDrawBitmapString(format_number(1), -60, 5);
                ofDrawBitmapString(format_number(0.5), -60, plotHeight/4.+5);
                ofDrawBitmapString(format_number(0), -60, plotHeight/2.+5);
                ofDrawBitmapString(format_number(-0.5), -60, 3*plotHeight/4.+5);
                ofDrawBitmapString(format_number(-1), -60, plotHeight+5);
            }
            else if(soundStream.getNumInputChannels() == 2)
            {
                ofDrawBitmapString("Gain (dB)", -15, -15);
                ofDrawBitmapString(format_number(y_max), -60, 5);
                ofDrawBitmapString(format_number(2*y_max/3.+y_min/3.), -60, plotHeight/3.+5);
                ofDrawBitmapString(format_number(y_max/3.+2*y_min/3.), -60, 2*plotHeight/3.+5);
                ofDrawBitmapString(format_number(y_min), -60, plotHeight+5);
            }
        ofPopMatrix();
    ofPopStyle();

    gui.draw();

    // bottom corner information display
    std::stringstream msg1;
    std::stringstream msg2;
    std::stringstream msg3;
    msg1 << "Channels: " << soundStream.getNumInputChannels();
    msg2 << "Buffer Size: " << soundStream.getBufferSize();
    msg3 << "Sample Rate: " << soundStream.getSampleRate();
    ofDrawBitmapString(msg1.str(), 2,ofGetHeight()-28);
    ofDrawBitmapString(msg2.str(), 2,ofGetHeight()-15);
    ofDrawBitmapString(msg3.str(), 2,ofGetHeight()-2);


}

//--------------------------------------------------------------
// this is the real-time audio loop
void ofApp::audioIn(ofSoundBuffer & input){
    if(!freeze)
        for (size_t i = 0; i < input.getNumFrames(); i++)
        {
            // if numchannels == 1, compression mode is off
            if(soundStream.getNumInputChannels() == 1)
                channel1.push_back(input[i]);
            // otherwise we are dividing by the second channel
            else if(soundStream.getNumInputChannels() == 2)
            {
                channel1.push_back(input[i*2]);
                channel2.push_back(input[i*2+1]);
            }
            // fill according to time window specification
            if(channel1.size() > 2*timeWidth*input.getSampleRate()/ofGetWidth())
            {
                // need to find max and min vals to display zoomed-out info
                std::vector<float> selections1 = findMaxAndMin(channel1,2);
                std::vector<float> selections2;
                if(soundStream.getNumInputChannels() == 2)
                    selections2 = findMaxAndMin(channel2,2);
                // record the max and min info
                for(int j = 0; j < selections1.size(); j++)
                    if(soundStream.getNumInputChannels() == 1)
                        volHistory.push_back(selections1[j]);
                    else if(soundStream.getNumInputChannels() == 2)
                        volHistory.push_back(selections1[j]/selections2[j]);
                channel1.clear();
                if(soundStream.getNumInputChannels() == 2)
                    channel2.clear();
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

