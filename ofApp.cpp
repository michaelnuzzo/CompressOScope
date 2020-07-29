 #include "ofApp.h"

// find max and min value in passed in audio chunk
std::vector<float> findMaxAndMin(std::vector<float> audioFrame,int numSelections, int size = -1)
{
    std::vector<float> selections;
    if(size == -1)
        size = audioFrame.size();
    else if (size == 0 || size > audioFrame.size())
        for(int i = 0; i < numSelections; i++)
            selections.push_back(0);
    else if(size > 0)
    {
        for(int i = 0; i < numSelections; i++)
            if(i%2==0)
                selections.push_back(*max_element(audioFrame.begin(), audioFrame.begin()+size));
            else
                selections.push_back(*min_element(audioFrame.begin(), audioFrame.begin()+size));
    }
    return selections;
}
// format the axis numbers
std::string format_number(float num, int precis = 2)
{
    std::stringstream stream;
    stream << std::right << std::setw(6) << std::fixed << std::setprecision(precis) << num;
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
void ofApp::changedCompMode(bool &isOn)
{
    auto& refCh = guiLeft.getGroup("Channels").getIntSlider("Reference Channel");
    auto& divCh = guiLeft.getGroup("Channels").getIntSlider("Divisor Channel");
    auto& refGain = guiLeft.getGroup("Channels").getFloatSlider("Ref. Gain (dB)");
    auto& divGain = guiLeft.getGroup("Channels").getFloatSlider("Div. Gain (dB)");
    auto& yMax = guiRight.getGroup("Compression Mode").getFloatSlider("Y max (dB)");
    auto& yMin = guiRight.getGroup("Compression Mode").getFloatSlider("Y min (dB)");

    if(isOn)
    {
        refGain.setBackgroundColor(black);
        refGain.setTextColor(black);
        refGain.setFillColor(black);
        divGain.setBackgroundColor(black);
        divGain.setTextColor(black);
        divGain.setFillColor(black);
        yMax.setBackgroundColor(black);
        yMax.setTextColor(white);
        yMax.setFillColor(gray);
        yMin.setBackgroundColor(black);
        yMin.setTextColor(white);
        yMin.setFillColor(gray);
    }
    else
    {
        refGain.setBackgroundColor(black);
        refGain.setTextColor(white);
        refGain.setFillColor(gray);
        divGain.setBackgroundColor(black);
        divGain.setTextColor(white);
        divGain.setFillColor(gray);
        yMax.setBackgroundColor(black);
        yMax.setTextColor(black);
        yMax.setFillColor(black);
        yMin.setBackgroundColor(black);
        yMin.setTextColor(black);
        yMin.setFillColor(black);
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
    ch1Gain = 0;
    ch2Gain = 0;
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
void ofApp::changedOffset(float &offset)
{
    setInputs = true;
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
    ofBackground(darkgray);

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
    channels.add(ch1Gain.set("Ref. Gain (dB)",0,0,120));
    channels.add(ch2Gain.set("Div. Gain (dB)",0,0,120));
    mainControls.add(timeWidth.set("time (s)",2.5,0.01,5));
    mainControls.add(offset.set("offset (ms)",0,-10,10));
    offset.addListener(this, &ofApp::changedOffset);
    compressionControls.add(compressionMode.set("Compression Analysis",false));
    compressionMode.addListener(this, &ofApp::changedCompMode);
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
    guiRight.getGroup("Compression Mode").getFloatSlider("Y max (dB)").setTextColor(black);
    guiRight.getGroup("Compression Mode").getFloatSlider("Y max (dB)").setFillColor(black);
    guiRight.getGroup("Compression Mode").getFloatSlider("Y min (dB)").setTextColor(black);
    guiRight.getGroup("Compression Mode").getFloatSlider("Y min (dB)").setFillColor(black);

    // set up the initial settings of the audio device
    settings.setInListener(this);
    settings.numOutputChannels = 0;
    settings.numInputChannels = device.inputChannels;
    soundStream.setup(settings);

    guiLeft.loadFromFile("settings-autosave-left.xml");
    guiRight.loadFromFile("settings-autosave-right.xml");
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
// this is the real-time graphics loop
void ofApp::draw(){


    ofSetColor(lightgray);
    ofNoFill();

    std::vector<std::pair<float,float>> values = volHistory; // a sad attempt to be more threadsafe...

    // draw the plot
    ofPushMatrix();
        ofTranslate(ofGetWidth()-plotWidth-ofGetWidth()/200., 1.2*ofGetHeight()/2.-plotHeight/2.);
        ofDrawRectangle(0, 0, plotWidth, plotHeight);
        // draw the waveform
            ofPushStyle();
                if(compressionMode)
                {
                    ofSetColor(green);
                    ofBeginShape();
                    for (unsigned int i = 0; i < values.size(); i++)
                        ofVertex(i, ofMap(toDb(values[i].first/values[i].second),-y_max,-y_min,0,plotHeight,true)); // compressor gain
                    ofEndShape(false);
                }
                else
                {
                    ofSetColor(red);
                    ofBeginShape();
                    for (unsigned int i = 0; i < values.size(); i++)
                        ofVertex(i, ofClamp(plotHeight/2.-values[i].first*plotHeight*toMag(ch1Gain),0,plotHeight));
                    ofEndShape(false);

                    ofSetColor(blue);
                    ofBeginShape();
                    for (unsigned int i = 0; i < values.size(); i++)
                        ofVertex(i, ofClamp(plotHeight/2.-values[i].second*plotHeight*toMag(ch2Gain),0,plotHeight));
                    ofEndShape(false);
                }
            ofPopStyle();


        // x-axis
        ofDrawBitmapString("Time (s)", plotWidth/2.-20, plotHeight+48);
        ofDrawBitmapString("0", plotWidth-8, plotHeight+20);
        ofDrawBitmapString(format_number(timeWidth/4.,3), 3*plotWidth/4.-35, plotHeight+20);
        ofDrawBitmapString(format_number(timeWidth/2.,3), plotWidth/2.-35, plotHeight+20);
        ofDrawBitmapString(format_number(3*timeWidth/4.,3), plotWidth/4.-35, plotHeight+20);
        ofDrawBitmapString(format_number(timeWidth,3), -35, plotHeight+20);

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
        auto& devSel = guiLeft.getGroup("Audio Interface").getIntSlider("Audio Device Selector");
        auto& splSel = guiLeft.getGroup("Audio Interface").getIntSlider("Sample Rate Selector");
        auto& bufSel = guiLeft.getGroup("Audio Interface").getIntSlider("Buffer Size");

        ofDrawBitmapString(soundStream.getDeviceList().at(audioDevice).name, devSel.getPosition().x+guiLeft.getWidth()+5, devSel.getPosition().y+14);
        ofDrawBitmapString(device.sampleRates.at(sampleRate), splSel.getPosition().x+guiLeft.getWidth()+5, splSel.getPosition().y+14);
        ofDrawBitmapString(to_string(int(pow(2.,float(bufferSize)))), bufSel.getPosition().x+guiLeft.getWidth()+5, bufSel.getPosition().y+14);
    }
    if(!guiLeft.getGroup("Channels").isMinimized() && !compressionMode)
    {
        auto& refCh = guiLeft.getGroup("Channels").getIntSlider("Reference Channel");
        auto& divCh = guiLeft.getGroup("Channels").getIntSlider("Divisor Channel");
        auto& refGain = guiLeft.getGroup("Channels").getFloatSlider("Ref. Gain (dB)");
        auto& divGain = guiLeft.getGroup("Channels").getFloatSlider("Div. Gain (dB)");

        ofPushStyle();
            ofFill();
            ofSetColor(red);
            ofDrawRectangle(refCh.getPosition().x+guiLeft.getWidth()+5, refCh.getPosition().y+5, 10, 10);
            ofDrawRectangle(refGain.getPosition().x+guiLeft.getWidth()+5, refGain.getPosition().y+5, 10, 10);

            ofSetColor(blue);
            ofDrawRectangle(divCh.getPosition().x+guiLeft.getWidth()+5, divCh.getPosition().y+5, 10, 10);
            ofDrawRectangle(divGain.getPosition().x+guiLeft.getWidth()+5, divGain.getPosition().y+5, 10, 10);
        ofPopStyle();
    }
    if(!guiRight.getGroup("Compression Mode").isMinimized() && compressionMode)
    {
        auto& yMax = guiRight.getGroup("Compression Mode").getFloatSlider("Y max (dB)");
        auto& yMin = guiRight.getGroup("Compression Mode").getFloatSlider("Y min (dB)");

        ofPushStyle();
            ofFill();
            ofSetColor(green);
            ofDrawRectangle(yMax.getPosition().x-23, yMax.getPosition().y+5, 10, 10);
            ofDrawRectangle(yMin.getPosition().x-23, yMin.getPosition().y+5, 10, 10);
        ofPopStyle();
    }
    if(!guiRight.getGroup("Main").isMinimized() && int(offset/1000.*soundStream.getSampleRate()) != 0)
    {
        auto& off = guiRight.getGroup("Main").getFloatSlider("offset (ms)");

        ofDrawBitmapString(to_string(int(offset/1000.*soundStream.getSampleRate()))+" samples", off.getPosition().x-115, off.getPosition().y+14);
    }

    guiLeft.draw();
    guiRight.draw();
}

//--------------------------------------------------------------
// this is the real-time audio loop
void ofApp::audioIn(ofSoundBuffer & input){

    if(setInputs)
    {
        input1.clear();
        input2.clear();

        if(offset < 0)
            input1.assign(int(abs(2*offset/1000.*soundStream.getSampleRate())),0);
        else if(offset > 0)
            input2.assign(int(abs(2*offset/1000.*soundStream.getSampleRate())),0);
        setInputs = false;

    }

    int timeSlice = 1+timeWidth*input.getSampleRate()/ofGetWidth();
    if(!freeze)
        for (size_t i = 0; i < input.getNumFrames(); i++)
        {
            input1.push_back(input[i*device.inputChannels+(input1Channel-1)]);
            input2.push_back(input[i*device.inputChannels+input2Channel-1]);
            while(input1.size() > 2*timeSlice && input2.size() > 2*timeSlice)
            {
                input1.erase(input1.begin(),input1.begin()+1);
                input2.erase(input2.begin(),input2.begin()+1);
            }

            // fill according to time window specification
            if(sampleCount % (2*timeSlice) == 0 && sampleCount > 0)
            {
                
                // need to find max and min vals to display zoomed-out info
                std::vector<float> selections1 = findMaxAndMin(input1,2,2*timeSlice);
                std::vector<float> selections2 = findMaxAndMin(input2,2,2*timeSlice);

                // record the max and min info
                for(int j = 0; j < 2; j++)
                    volHistory.push_back(std::make_pair(selections1[j],selections2[j]));

                //if we are bigger the the size we want to record - lets drop the oldest value
                if( volHistory.size() >= plotWidth)
                    volHistory.erase(volHistory.begin(), volHistory.begin()+2);
            }
            sampleCount++;
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
