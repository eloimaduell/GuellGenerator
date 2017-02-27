//
//  parametersControl.cpp
//  DGTL_Generator
//
//  Created by Eduard Frigola on 03/08/16.
//
//

#include "parametersControl.h"
#include <random>

void parametersControl::createGuiFromParams(ofParameterGroup paramGroup, ofColor guiColor){
    
    ofxDatGuiLog::quiet();
    ofxDatGui::setAssetPath("");
    
    //Put parameterGroup into vector
    parameterGroups.push_back(paramGroup);
        
    ofxDatGui* tempDatGui = new ofxDatGui();
    
    ofxDatGuiTheme* theme = new ofxDatGuiThemeCharcoal;
    theme->color.slider.fill = guiColor;
    theme->color.textInput.text = guiColor;
    theme->color.icons = guiColor;
    tempDatGui->setTheme(theme);
    
    tempDatGui->setWidth(290);
    tempDatGui->addHeader(paramGroup.getName());
    tempDatGui->addFooter();
    if(datGuis.size() == 0)
        tempDatGui->setPosition(0, 0);
    else
        tempDatGui->setPosition((datGuis[datGuis.size()-1]->getPosition().x + datGuis[datGuis.size()-1]->getWidth()), 0);
    
    for(int i=0 ; i<paramGroup.size(); i++){
        ofAbstractParameter &absParam = paramGroup.get(i);
        if(absParam.type() == typeid(ofParameter<float>).name())
            tempDatGui->addSlider(paramGroup.getFloat(i));
        else if(absParam.type() == typeid(ofParameter<int>).name())
            tempDatGui->addSlider(paramGroup.getInt(i));
        else if(absParam.type() == typeid(ofParameter<bool>).name())
            tempDatGui->addToggle(paramGroup.getName(i))->setChecked(paramGroup.getBool(i).get());
        else if(absParam.type() == typeid(ofParameter<string>).name()){
            if(ofSplitString(absParam.getName(), "_").size() > 1)
                tempDatGui->addLabel(ofSplitString(absParam.getName(), "_")[0]);
            else
                tempDatGui->addTextInput(absParam.getName(), absParam.cast<string>());
        }
        else if(absParam.type() == typeid(ofParameter<ofColor>).name())
            tempDatGui->addColorPicker(paramGroup.getName(i), ofColor::white);
        else if(absParam.type() == typeid(ofParameterGroup).name()){
            tempDatGui->addLabel(paramGroup.getGroup(i).getName());
            tempDatGui->addDropdown(paramGroup.getGroup(i).getName(), ofSplitString(paramGroup.getGroup(i).getString(0), "-|-"))->select(paramGroup.getGroup(i).getInt(1));
        }else{
            tempDatGui->addLabel(absParam.getName());
        }
    }
    datGuis.push_back(tempDatGui);
    
    if(paramGroup.getName() == "master 1")
        masterGroupParam = &parameterGroups.back();
}

void parametersControl::setup(){
    
    ofRegisterKeyEvents(this);
    ofRegisterMouseEvents(this);
    //DatGui
    
    ofxDatGuiLog::quiet();
    ofxDatGui::setAssetPath("");
    
    datGui = new ofxDatGui();
    
    ofxDatGuiTheme* theme = new ofxDatGuiThemeCharcoal;
    ofColor randColor =  ofColor::indianRed;
    theme->color.slider.fill = randColor;
    theme->color.textInput.text = randColor;
    theme->color.icons = randColor;
    datGui->setTheme(theme);
    
    datGui->setWidth(290);
    datGui->addHeader("Presets Control");
    datGui->addFooter();

    datGui->addButton("Global Reset Phase");
    datGui->addSlider("Global BPM", 0, 999, 120);
    
    //Preset Control
    bankSelect = datGui->addDropdown("Bank Select", {"edu", "eloi", "santi"});
    presetMatrix = datGui->addMatrix("Presets", NUM_PRESETS, true);
    presetMatrix->setRadioMode(true);
    presetMatrix->setOpacity(.75);
    
    presetMatrix->onMatrixEvent(this, &parametersControl::onGuiMatrixEvent);
    
    datGui->addToggle("BPM Sync")->setChecked(false);
    datGui->addToggle("Automatic Preset");
    datGui->addButton("Reload Sequence");
    datGui->addSlider(fadeTime.set("Fade Time", 1, 0, 10));
//    datGui->addSlider(presetChangeBeatsPeriod.set("Beats Period", 4, 1, 120));
    
    ofPoint guiPos = datGuis[0]->getPosition() + ofPoint(0, datGuis[0]->getWidth());
    datGui->setPosition(guiPos.x, guiPos.y);
    
    //ControlGui Events
    datGui->onButtonEvent(this, &parametersControl::onGuiButtonEvent);
    datGui->onToggleEvent(this, &parametersControl::onGuiToggleEvent);
    datGui->onSliderEvent(this, &parametersControl::onGuiSliderEvent);
    datGui->onDropdownEvent(this, &parametersControl::onGuiDropdownEvent);

    //GUIS EVENT LISTERNERS
    for(auto gui : datGuis){
        gui->onButtonEvent(this, &parametersControl::onGuiButtonEvent);
        gui->onToggleEvent(this, &parametersControl::onGuiToggleEvent);
        gui->onDropdownEvent(this, &parametersControl::onGuiDropdownEvent);
//        gui->onSliderEvent(this, &parametersControl::onGuiSliderEvent);
        gui->onTextInputEvent(this, &parametersControl::onGuiTextInputEvent);
        gui->onColorPickerEvent(this, &parametersControl::onGuiColorPickerEvent);
        gui->onRightClickEvent(this, &parametersControl::onGuiRightClickEvent);
    }
    
    //OF PARAMETERS LISTERENRS
    for(auto paramGroup : parameterGroups){
        ofAddListener(paramGroup.parameterChangedE(), this, &parametersControl::listenerFunction);
    }
    
    //OSC
    ofLog() << "OSC Info:";
    ofXml xml;
    if(xml.load("OSCConfig.xml")){
        string host = xml.getValue("host");
        if(host != ""){
            oscReceiver.setup(xml.getIntValue("ReceiverPort"));
            oscSender.setup(host, xml.getIntValue("SenderPort"));
            ofLog() << "Listening on port " << 12345;
            ofLog() << "Sending to host: " << host << " on port: " << 54321;
        }else
            ofLog() << "NO OSC";
    }
    
    //MIDI
    ofLog() << "MIDI Info:";
    ofxMidiOut::listPorts();
    for(int i = 0; i< ofxMidiOut::getNumPorts(); i++){
        midiOut.push_back(ofxMidiOut());
        midiOut.back().openPort(i);
    }

    
    ofxMidiIn::listPorts();
    for(int i = 0; i< ofxMidiIn::getNumPorts(); i++){
        midiIn.push_back(ofxMidiIn());
        midiIn.back().openPort(i);
        midiIn.back().addListener(this);
    }
    
    autoPreset = false;
    
    datGui->getButton("Reload Sequence")->setBackgroundColor(loadPresetsSequence() ? ofColor::green : ofColor::red);
    
    Tweenzor::init();
    
    beatTracker = &bpmControl::getInstance();
    beatTracker->setup();
    
    loadMidiMapping();
}


void parametersControl::update(){
    Tweenzor::update(ofGetElapsedTimeMillis());
    
    while(oscReceiver.hasWaitingMessages()){
        ofxOscMessage m;
        oscReceiver.getNextMessage(m);
        
        vector<string> splitAddress = ofSplitString(m.getAddress(), "/");
        if(splitAddress.size() >= 2){
            ofStringReplace(splitAddress[1], "_", " ");
            if(splitAddress[1] == "presetLoad"){
                loadPreset(m.getArgAsInt(0), m.getArgAsString(1));
            }else if(splitAddress[1] == "presetSave"){
                savePreset(m.getArgAsInt(0), m.getArgAsString(1));
            }else if(splitAddress[1] == "phaseReset"){
                for(auto groupParam : parameterGroups){
                    if(ofStringTimesInString(groupParam.getName(), "phasor") != 0)
                        groupParam.getBool("Reset Phase") = 0;
                }
            }else{
                for(auto groupParam : parameterGroups){
                    if(groupParam.getName() == splitAddress[1]){
                        ofAbstractParameter &absParam = groupParam.get(splitAddress[2]);
                        if(absParam.type() == typeid(ofParameter<float>).name()){
                            ofParameter<float> castedParam = absParam.cast<float>();
                            castedParam = ofMap(m.getArgAsFloat(0), 0, 1, castedParam.getMin(), castedParam.getMax(), true);
                        }else if(absParam.type() == typeid(ofParameter<int>).name()){
                            ofParameter<int> castedParam = absParam.cast<int>();
                            castedParam = ofMap(m.getArgAsFloat(0), 0, 1, castedParam.getMin(), castedParam.getMax(), true);
                        }else if(absParam.type() == typeid(ofParameter<bool>).name())
                            groupParam.getBool(splitAddress[2]) = m.getArgAsBool(0);
                        else if(absParam.type() == typeid(ofParameter<string>).name())
                            groupParam.getString(splitAddress[2]) = m.getArgAsString(0);
                        else
                            groupParam.getGroup(splitAddress[2]).getInt(1) = m.getArgAsInt(0); //DropDown
                    }
                }
            }
        }
    }
    
    if(isFading){
        ofxOscMessage m;
        m.setAddress("presetFade");
        m.addFloatArg(masterGroupParam->getFloat("Master Fader").get());
        oscSender.sendMessage(m);
    }
    
    //MIDI - Process the midi deque
    for(auto m : midiMessagesFiller)
        midiMessages.push_front(m);
    midiMessagesFiller.clear();
    
    while(midiMessages.size() > 0){
        if(midiMessages[0].portName != "Circuit"){
            int parameterChan = midiMessages[0].channel;
            int parameterNum = midiMessages[0].status == MIDI_NOTE_ON ? midiMessages[0].pitch : midiMessages[0].control;
            int parameterVal = midiMessages[0].status == MIDI_NOTE_ON ? midiMessages[0].velocity : midiMessages[0].value;
            string parameterPort = midiMessages[0].portName;
            
            for (auto &connection : midiIntConnections){
                if(connection.isListening())
                    connection.assignMidiControl(parameterPort, parameterChan, parameterNum);
                else if(connection.getDevice() == parameterPort && connection.getChannel() == parameterChan && connection.getControl() == parameterNum){
                    connection.setValue(parameterVal);
                }
            }
            for (auto &connection : midiFloatConnections){
                if(connection.isListening())
                    connection.assignMidiControl(parameterPort, parameterChan, parameterNum);
                else if(connection.getDevice() == parameterPort && connection.getChannel() == parameterChan && connection.getControl() == parameterNum){
                    connection.setValue(parameterVal);
                }
            }
            for (auto &connection : midiBoolConnections){
                if(connection.isListening())
                    connection.assignMidiControl(parameterPort, parameterChan, parameterNum);
                else if(connection.getDevice() == parameterPort && connection.getChannel() == parameterChan && connection.getControl() == parameterNum){
                    connection.setValue(parameterVal);
                }
            }
        }
        else if(midiMessages[0].status == MIDI_NOTE_ON){
            int pitch = midiMessages[0].pitch;
            int presetToTrigger = ofFind(pitchMapper, pitch)+1;
            
            midiOut[midiMessages[0].portNum].sendNoteOn(midiMessages[0].channel, pitch);
            midiOut[midiMessages[0].portNum].sendNoteOff(midiMessages[0].channel, lastMidiPressetPitch);
            lastMidiPressetPitch = pitch;
            
            loadPresetWithFade(presetToTrigger, bankSelect->getSelected()->getName());
        }
        
        midiMessages.pop_front();
    }
    
    //Auto preset
    if(randomPresetsArrange.size()>0 && autoPreset && (ofGetElapsedTimef()-presetChangedTimeStamp) > periodTime){
        presetChangedTimeStamp = presetChangedTimeStamp+periodTime;
        int index = randomPresetsArrange[presetChangeCounter];
        loadPresetWithFade(presetNumbersAndBanks.at(index).first, presetNumbersAndBanks.at(index).second);
        periodTime = presetsTime[index];
        presetChangeCounter++;
        if(presetChangeCounter >= randomPresetsArrange.size()){
            presetChangeCounter = 0;
            mt19937 g(static_cast<uint32_t>(time(0)));
            shuffle(randomPresetsArrange.begin(), randomPresetsArrange.end(), g);
        }
    }
    
    if(newBpm != 0){
        datGui->getSlider("Global BPM")->setValue(newBpm);
        for(auto groupParam : parameterGroups){
            if(ofStringTimesInString(groupParam.getName(), "phasor") != 0 && groupParam.getFloat("BPM").getName() == "BPM")
                groupParam.getFloat("BPM") = newBpm;
        }
        newBpm = 0;
    }

}

void parametersControl::draw(){
    ofPushStyle();
    ofSetColor(ofColor::white);
    ofSetLineWidth(2);
    for(auto connection : connections){
        connection.getPolyline().draw();
    }
    ofPopStyle();
}

void parametersControl::saveGuiArrangement(){
    //xml.load("Preset_"+ofToString(presetNum)+".xml");
    xml.clear();
    
    //Root Element
    xml.addChild("GUIARRANGEMENT");
    
    // now we set our current element so we're on the right
    // element, so when add new nodes, they are still inside
    //the root element
    xml.setTo("GUIARRANGEMENT");
    
    xml.addChild("GuiWindow");
    xml.setTo("GuiWindow");
    xml.addValue("x", guiWindow->getWindowPosition().x);
    xml.addValue("y", guiWindow->getWindowPosition().y);
    xml.addValue("width", guiWindow->getWindowSize().x);
    xml.addValue("height", guiWindow->getWindowSize().y);
    xml.setToParent();
    
    xml.addChild("PrevWindow");
    xml.setTo("PrevWindow");
    xml.addValue("x", prevWindow->getWindowPosition().x);
    xml.addValue("y", prevWindow->getWindowPosition().y);
    xml.addValue("width", prevWindow->getWindowSize().x);
    xml.addValue("height", prevWindow->getWindowSize().y);
    xml.setToParent();
    
    xml.addChild("Main");
    xml.setTo("Main");
    xml.addValue("x", ofToString(datGui->getPosition().x));
    xml.addValue("y", ofToString(datGui->getPosition().y));
    xml.setToParent();
    
    for ( int i = 0; i < datGuis.size() ; i++){
        xml.addChild("Gui_" + ofToString(i));
        xml.setTo("Gui_" + ofToString(i));
        xml.addValue("x", ofToString(datGuis[i]->getPosition().x));
        xml.addValue("y", ofToString(datGuis[i]->getPosition().y));
        xml.setToParent();
    }
    xml.save("GuiArrangement_"+bankSelect->getSelected()->getName()+".xml");
}

void parametersControl::loadGuiArrangement(){
    //Test if there is no problem with the file
    if(!xml.load("GuiArrangement_"+bankSelect->getSelected()->getName()+".xml"))
        return;
    
    if(xml.exists("GuiWindow")){
        xml.setTo("GuiWindow");
        guiWindow->setWindowPosition(xml.getIntValue("x"), xml.getIntValue("y"));
        guiWindow->setWindowShape(xml.getIntValue("width"), xml.getIntValue("height"));
        xml.setToParent();
    }
    
    if(xml.exists("PrevWindow")){
        xml.setTo("PrevWindow");
        prevWindow->setWindowPosition(xml.getIntValue("x"), xml.getIntValue("y"));
        prevWindow->setWindowShape(xml.getIntValue("width"), xml.getIntValue("height"));
        xml.setToParent();
    }

    if(xml.exists("Main")){
        xml.setTo("Main");
        datGui->setPosition(xml.getIntValue("x"), xml.getIntValue("y"));
        xml.setToParent();
    }

    for ( int i = 0; i < datGuis.size() ; i++){
        if(xml.exists("Gui_" + ofToString(i))){
            xml.setTo("Gui_" + ofToString(i));
            datGuis[i]->setPosition(xml.getIntValue("x"), xml.getIntValue("y"));
            xml.setToParent();
        }
    }
    
    ofLog()<<"Load Arrangement_" + bankSelect->getSelected()->getName();
}

void parametersControl::saveMidiMapping(){
    ofXml xml2;
    xml2.clear();
    
    //Root Element
    xml2.addChild("MIDIMAPPING");
    
    // now we set our current element so we're on the right
    // element, so when add new nodes, they are still inside
    //the root element
    xml2.setTo("MIDIMAPPING");
    
    for (int i = 0; i < midiIntConnections.size(); i++){
        xml2.addChild("MidiInt_" + ofToString(i));
        xml2.setTo("MidiInt_" + ofToString(i));
        if(midiIntConnections[i].getParameter()->getGroupHierarchyNames().size() == 3){
            xml2.addValue("isDropdown", "true");
            string sGroup = midiIntConnections[i].getParameter()->getGroupHierarchyNames()[0];
            ofStringReplace(sGroup, "_", " ");
            xml2.addValue("SourceGroup", sGroup);
            xml2.addValue("SourceName", midiIntConnections[i].getParameter()->getFirstParent().getName());
        }else{
            xml2.addValue("isDropdown", "false");
            xml2.addValue("SourceGroup", midiIntConnections[i].getParameter()->getFirstParent().getName());
            xml2.addValue("SourceName", midiIntConnections[i].getParameter()->getName());
        }
        xml2.addValue("MidiDevice", midiIntConnections[i].getDevice());
        xml2.addValue("MidiChannel", ofToString(midiIntConnections[i].getChannel()));
        xml2.addValue("MidiControl", ofToString(midiIntConnections[i].getControl()));
        xml2.setToParent();
    }
    
    for (int i = 0; i < midiFloatConnections.size(); i++){
        xml2.addChild("MidiFloat_" + ofToString(i));
        xml2.setTo("MidiFloat_" + ofToString(i));
        xml2.addValue("SourceGroup", midiFloatConnections[i].getParameter()->getFirstParent().getName());
        xml2.addValue("SourceName", midiFloatConnections[i].getParameter()->getName());
        xml2.addValue("MidiDevice", midiFloatConnections[i].getDevice());
        xml2.addValue("MidiChannel", ofToString(midiFloatConnections[i].getChannel()));
        xml2.addValue("MidiControl", ofToString(midiFloatConnections[i].getControl()));
        xml2.setToParent();
    }
    
    for (int i = 0; i < midiBoolConnections.size(); i++){
        xml2.addChild("MidiBool_" + ofToString(i));
        xml2.setTo("MidiBool_" + ofToString(i));
        xml2.addValue("SourceGroup", midiBoolConnections[i].getParameter()->getFirstParent().getName());
        xml2.addValue("SourceName", midiBoolConnections[i].getParameter()->getName());
        xml2.addValue("MidiDevice", midiBoolConnections[i].getDevice());
        xml2.addValue("MidiChannel", ofToString(midiBoolConnections[i].getChannel()));
        xml2.addValue("MidiControl", ofToString(midiBoolConnections[i].getControl()));
        xml2.addValue("MidiToggleMode", ofToString(midiBoolConnections[i].isToggle()));
        xml2.setToParent();
    }
    
    xml2.save("MidiMapping.xml");
}

void parametersControl::loadMidiMapping(){
    //Test if there is no problem with the file
    if(!xml.load("MidiMapping.xml"))
        return;
    
    int i = 0;
    while(xml.exists("MidiInt_" + ofToString(i))){
        xml.setTo("MidiInt_" + ofToString(i));
        string isDrop = xml.getValue("isDropdown");
        for(auto &paramGroup : parameterGroups){
            if(paramGroup.getName() == xml.getValue("SourceGroup")){
                if(isDrop == "true")
                    midiIntConnections.push_back(midiConnection<int>(&paramGroup.getGroup(xml.getValue("SourceName")).getInt(1)));
                else
                    midiIntConnections.push_back(midiConnection<int>(&paramGroup.getInt(xml.getValue("SourceName"))));
            }
            
        }
        midiIntConnections.back().assignMidiControl(xml.getValue("MidiDevice"), xml.getIntValue("MidiChannel"), xml.getIntValue("MidiControl"));
        xml.setToParent();
        i++;
    }
    i = 0;
    while(xml.exists("MidiFloat_" + ofToString(i))){
        xml.setTo("MidiFloat_" + ofToString(i));
        for(auto &paramGroup : parameterGroups){
            if(paramGroup.getName() == xml.getValue("SourceGroup"))
                midiFloatConnections.push_back(midiConnection<float>(&paramGroup.getFloat(xml.getValue("SourceName"))));
        }
        midiFloatConnections.back().assignMidiControl(xml.getValue("MidiDevice"), xml.getIntValue("MidiChannel"), xml.getIntValue("MidiControl"));
        xml.setToParent();
        i++;
    }
    i = 0;
    while(xml.exists("MidiBool_" + ofToString(i))){
        xml.setTo("MidiBool_" + ofToString(i));
        for(auto &paramGroup : parameterGroups){
            if(paramGroup.getName() == xml.getValue("SourceGroup"))
                midiBoolConnections.push_back(midiConnection<bool>(&paramGroup.getBool(xml.getValue("SourceName"))));
        }
        midiBoolConnections.back().assignMidiControl(xml.getValue("MidiDevice"), xml.getIntValue("MidiChannel"), xml.getIntValue("MidiControl"));
        midiBoolConnections.back().setToggle(xml.getBoolValue("MidiToggleMode"));
            
        xml.setToParent();
        i++;
    }

    ofLog()<<"Load Midi Mapping";
}

bool parametersControl::loadPresetsSequence(){
    // this is our buffer to stroe the text data
    ofBuffer buffer = ofBufferFromFile("PresetsSequencing.txt");
    
    if(buffer.size()) {
        presetNumbersAndBanks.clear();
        presetsTime.clear();
        for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
            string line = *it;
            
            // make sure its not a empty line
            if(!line.empty()){
                vector<string> splitedStr = ofSplitString(line,"-");
                pair<int, string> tempPair;
                tempPair.first = ofToInt(splitedStr[0]);
                tempPair.second = splitedStr[1];
                presetNumbersAndBanks.push_back(tempPair);
                presetsTime.push_back(ofToInt(splitedStr[2]));
            }
        }
    }
    else{
        return false;
    }
    
    if(presetNumbersAndBanks.size() != presetsTime.size()){
        presetNumbersAndBanks.clear();
        presetsTime.clear();
        return false;
    }
    
    randomPresetsArrange.clear();
    for(int i = 0 ; i < presetNumbersAndBanks.size(); i++)
        randomPresetsArrange.push_back(i);
    
    presetChangeCounter = 0;
    presetChangedTimeStamp = ofGetElapsedTimef();
    
    mt19937 g(static_cast<uint32_t>(time(0)));
    shuffle(randomPresetsArrange.begin(), randomPresetsArrange.end(), g);
    
    return true;
}

void parametersControl::savePreset(int presetNum, string bank){
    //xml.load("Preset_"+ofToString(presetNum)+".xml");
    xml.clear();
    
    //Root Element
    xml.addChild("PRESET");
    
    // now we set our current element so we're on the right
    // element, so when add new nodes, they are still inside
    //the root element
    xml.setTo("PRESET");
    
    //Iterate for all the three parameterGroups
    for (auto groupParam : parameterGroups){
        //set XML structure to parameterGroup
        string noSpacesGroupName = groupParam.getName();
        ofStringReplace(noSpacesGroupName, " ", "_");
        xml.addChild(noSpacesGroupName);
        xml.setTo(noSpacesGroupName);
        
        //Iterate for all parameters in parametergroup and look for the type of the parameter
        for (int j = 0; j < groupParam.size() ; j++){
            ofAbstractParameter &absParam = groupParam.get(j);
            if(absParam.type() == typeid(ofParameter<float>).name()){
                //Cast it
                ofParameter<float> castedParam = absParam.cast<float>();
                
                //Replace blank spaces with underscore
                string noSpaces = castedParam.getName();
                ofStringReplace(noSpaces, " ", "_");
                
                //add the value of that parameter into xml
                xml.addValue(noSpaces, castedParam.get());
            }
            else if(absParam.type() == typeid(ofParameter<int>).name()){
                ofParameter<int> castedParam = absParam.cast<int>();
                string noSpaces = castedParam.getName();
                ofStringReplace(noSpaces, " ", "_");
                xml.addValue(noSpaces, castedParam.get());
            }
            else if(absParam.type() == typeid(ofParameter<bool>).name()){
                ofParameter<bool> castedParam = absParam.cast<bool>();
                string noSpaces = castedParam.getName();
                ofStringReplace(noSpaces, " ", "_");
                xml.addValue(noSpaces, castedParam.get());
            }
            else if(absParam.type() == typeid(ofParameter<string>).name()){
                ofParameter<string> castedParam = absParam.cast<string>();
                string noSpaces = castedParam.getName();
                ofStringReplace(noSpaces, " ", "_");
                xml.addValue(noSpaces, castedParam.get());
            }
            else if(absParam.type() == typeid(ofParameter<ofColor>).name()){
                ofParameter<ofColor> castedParam = absParam.cast<ofColor>();
                string noSpaces = castedParam.getName();
                ofStringReplace(noSpaces, " ", "_");
                xml.addValue(noSpaces, ofToString((int)castedParam.get().r) + "-" + ofToString((int)castedParam.get().g) + "-" + ofToString((int)castedParam.get().b));
            }
            else{
                string noSpaces = groupParam.getGroup(j).getName();
                ofStringReplace(noSpaces, " ", "_");
                xml.addValue(noSpaces, groupParam.getGroup(j).getInt(1).get());
            }
        }
        xml.setToParent();
    }
    ofLog() <<"Save Preset_" << presetNum<< "_" << bank;
    xml.save("Preset_"+ofToString(presetNum)+"_"+bank+".xml");
    
    ofxOscMessage m;
    m.setAddress("presetSave");
    m.addIntArg(presetNum);
    m.addStringArg(bank);
    oscSender.sendMessage(m);
}

void parametersControl::loadPreset(int presetNum, string bank){
    //Test if there is no problem with the file
//    xml.clear();
    
    bool isColorLoaded = false;

    
    if(!xml.load("Preset_"+ofToString(presetNum)+"_"+bank+".xml"))
        return;
    
    //Iterate for all the parameterGroups
    for (auto groupParam : parameterGroups){
        //Put xml in the place of the parametergroup
        string noSpacesGroupName = groupParam.getName();
        ofStringReplace(noSpacesGroupName, " ", "_");
        if(xml.exists(noSpacesGroupName) && noSpacesGroupName != "senderManager_1"){
            xml.setTo(noSpacesGroupName);
            
            //Iterate for all parameters in parametergroup and look for the type of the parameter
            for (int j = 0; j < groupParam.size() ; j++){
                ofAbstractParameter &absParam = groupParam.get(j);
                if(absParam.type() == typeid(ofParameter<float>).name()){
                    //Cast it
                    ofParameter<float> *castedParam = &absParam.cast<float>();
                    
                    //Replace blank spaces with underscore
                    string noSpaces = castedParam->getName();
                    ofStringReplace(noSpaces, " ", "_");
                    
                    //get the value of that parameter if it's not bpm, we don't want to lose sync
                    if(castedParam->getName() != "BPM" && xml.exists(noSpaces)  && !ofStringTimesInString(groupParam.getName(), "master"))
                        castedParam->set(xml.getValue(noSpaces, castedParam->get()));
                }
                else if(absParam.type() == typeid(ofParameter<int>).name()){
                    ofParameter<int> *castedParam = &absParam.cast<int>();
                    string noSpaces = castedParam->getName();
                    ofStringReplace(noSpaces, " ", "_");
                    if(xml.exists(noSpaces) && !ofStringTimesInString(groupParam.getName(), "master"))
                        castedParam->set(xml.getValue(noSpaces, castedParam->get()));
                }
                else if(absParam.type() == typeid(ofParameter<bool>).name()){
                    ofParameter<bool> *castedParam = &absParam.cast<bool>();
                    string noSpaces = castedParam->getName();
                    ofStringReplace(noSpaces, " ", "_");
                    if(xml.exists(noSpaces) && !ofStringTimesInString(groupParam.getName(), "master"))
                        castedParam->set(xml.getValue(noSpaces, castedParam->get()));
                }
                else if(absParam.type() == typeid(ofParameter<string>).name()){
                    ofParameter<string> *castedParam = &absParam.cast<string>();
                    string noSpaces = castedParam->getName();
                    ofStringReplace(noSpaces, " ", "_");
                    if(xml.exists(noSpaces) && !ofStringTimesInString(groupParam.getName(), "master"))
                        castedParam->set(xml.getValue(noSpaces, castedParam->get()));
                }
                else if(absParam.type() == typeid(ofParameter<ofColor>).name()){
                    ofParameter<ofColor> *castedParam = &absParam.cast<ofColor>();
                    string noSpaces = castedParam->getName();
                    ofStringReplace(noSpaces, " ", "_");
                    if(xml.exists(noSpaces) && masterGroupParam->getBool("Load Color Preset")){
                        vector<string> colors = ofSplitString(xml.getValue(noSpaces), "-");
                        ofColor_<unsigned char> color = ofColor(ofToInt(colors[0]), ofToInt(colors[1]), ofToInt(colors[2]));
                        //TODO: hack, arreglar
                        groupParam.getInt("R Channel") = ofToInt(colors[0]);
                        groupParam.getInt("G Channel") = ofToInt(colors[1]);
                        groupParam.getInt("B Channel") = ofToInt(colors[2]);
                        isColorLoaded = true;
                    }
                }
                else{
                    string noSpaces = groupParam.getGroup(j).getName();
                    ofStringReplace(noSpaces, " ", "_");
                    if(xml.exists(noSpaces) && !ofStringTimesInString(groupParam.getName(), "master"))
                        groupParam.getGroup(j).getInt(1) = xml.getValue(noSpaces, groupParam.getGroup(j).getInt(1));
                }
            }
            //Jump one label before in xml structure
            xml.setToParent();
            //reset Phasor
            if(ofSplitString(groupParam.getName(), " ")[0] == "phasor")
                groupParam.getBool("Reset Phase") = false;
        }
    }
    
    if(!isColorLoaded && masterGroupParam->getBool("Randomize Color")){
        masterGroupParam->getBool("Trigger Random") = true;
    }
    
    
    ofLog()<<"Load Preset_" << presetNum<< "_" << bank;
    vector<int> tempVec;
    tempVec.push_back(presetNum-1);
    presetMatrix->setSelected(tempVec);
    
    ofxOscMessage m;
    m.setAddress("presetLoad");
    m.addIntArg(presetNum);
    m.addStringArg(bank);
    oscSender.sendMessage(m);
    
    for(auto groupParam : parameterGroups){
        if(ofStringTimesInString(groupParam.getName(), "phasor") != 0)
            groupParam.getBool("Reset_Phase") = 0;
    }
}

void parametersControl::loadPresetWithFade(int presetNum, string bank){
    ofXml xml2;
    if(xml2.load("Preset_"+ofToString(presetNum)+"_"+bank+".xml")){
        presetToLoad = presetNum;
        bankToLoad = bank;
        ofxOscMessage m;
        m.setAddress("bankSelect");
        m.addStringArg(bank);
        oscSender.sendMessage(m);
        Tweenzor::add((float*)&(masterGroupParam->getFloat("Master Fader").get()), masterGroupParam->getFloat("Master Fader").get(), 0.0f, 0.0f, fadeTime);
        Tweenzor::addCompleteListener(Tweenzor::getTween((float*)&masterGroupParam->getFloat("Master Fader").get()), this, &parametersControl::loadPresetWhenFadeOutCompletes);
        isFading = true;
    }
}

void parametersControl::loadPresetWhenFadeOutCompletes(float *arg){
    if(*arg == 0){
        loadPreset(presetToLoad, bankToLoad);
        Tweenzor::add((float*)&masterGroupParam->getFloat("Master Fader").get(), 0.0f, 1.0f, 0.0f, fadeTime);
         Tweenzor::addCompleteListener(Tweenzor::getTween((float*)&masterGroupParam->getFloat("Master Fader").get()), this, &parametersControl::loadPresetWhenFadeOutCompletes);
    }
    else if(*arg == 1.0f){
        isFading = false;
    }
}

void parametersControl::onGuiButtonEvent(ofxDatGuiButtonEvent e){
    if(datGui->getButton(e.target->getName())){
        if(ofStringTimesInString(e.target->getName(), "Global")){
            string nameNoGlobal = e.target->getName();
            ofStringReplace(nameNoGlobal, "Global ", "");
            for(auto groupParam : parameterGroups){
                if(ofStringTimesInString(groupParam.getName(), "phasor") != 0)
                    groupParam.getBool(nameNoGlobal) = 0;
            }
        }else if(e.target->getName() == "Reload Sequence"){
            e.target->setBackgroundColor(loadPresetsSequence() ? ofColor(0,50,0) : ofColor(50,0,0));
        }
    }
}
void parametersControl::onGuiToggleEvent(ofxDatGuiToggleEvent e){
    for (int i=0; i < datGuis.size() ; i++){
        if(datGuis[i]->getToggle(e.target->getName()) == e.target)
            parameterGroups[i].getBool(e.target->getName()) = e.target->getChecked();
    }
    if(e.target->getName() == "Automatic Preset"){
        autoPreset = e.checked;
        presetChangedTimeStamp = ofGetElapsedTimef();
        srand(time(0));
        random_shuffle(randomPresetsArrange.begin(), randomPresetsArrange.end());
    }else if(e.target->getName() == "BPM Sync"){
        if(e.checked)
            ofAddListener(beatTracker->bpmChanged, this, &parametersControl::bpmChangedListener);
        else
            ofRemoveListener(beatTracker->bpmChanged, this, &parametersControl::bpmChangedListener);
        
    }
}

void parametersControl::onGuiDropdownEvent(ofxDatGuiDropdownEvent e){
    if(e.target == bankSelect)
        loadGuiArrangement();
    else{
        for (int i=0; i < datGuis.size() ; i++){
            if(datGuis[i]->getDropdown(e.target->getName()) == e.target){
                parameterGroups[i].getGroup(e.target->getName()).getInt(1) = e.child;
                //            if(datGuis[i]->getHeight() > ofGetHeight())
                //                ofSetWindowShape(ofGetWidth(), datGuis[i]->getHeight());
            }
        }
    }
}

void parametersControl::onGuiMatrixEvent(ofxDatGuiMatrixEvent e){
    if(ofGetKeyPressed(OF_KEY_SHIFT))
        savePreset(e.child+1, bankSelect->getSelected()->getName());
    else{
        loadPresetWithFade(e.child+1, bankSelect->getSelected()->getName());
        if(autoPreset)
            presetChangedTimeStamp = ofGetElapsedTimef();
    }
}

void parametersControl::onGuiSliderEvent(ofxDatGuiSliderEvent e){
    if(datGui->getSlider(e.target->getName())->getName() == e.target->getName()){
        string nameNoGlobal = e.target->getName();
        ofStringReplace(nameNoGlobal, "Global ", "");
        for(auto groupParam : parameterGroups){
            if(ofStringTimesInString(groupParam.getName(), "phasor") != 0 && groupParam.getFloat(nameNoGlobal).getName() == nameNoGlobal)
                groupParam.getFloat(nameNoGlobal) = e.value;
        }
    }
    if(e.target->getName() == "Beats Period")
        periodTime = e.value / datGui->getSlider("Global BPM")->getValue() * 60.;
}

void parametersControl::onGuiTextInputEvent(ofxDatGuiTextInputEvent e){
    for (int i=0; i < datGuis.size() ; i++){
        if(datGuis[i]->getTextInput(e.target->getName()) == e.target)
            parameterGroups[i].getString(e.target->getName()) = e.text;
    }
}

void parametersControl::onGuiColorPickerEvent(ofxDatGuiColorPickerEvent e){
    for (int i=0; i < datGuis.size() ; i++){
        if(datGuis[i]->getColorPicker(e.target->getName()) == e.target)
            parameterGroups[i].getColor(e.target->getName()) = e.color;
    }
}

void parametersControl::onGuiRightClickEvent(ofxDatGuiRightClickEvent e){
    if(e.down == 1){
        for (int i=0; i < datGuis.size() ; i++){
            if(datGuis[i]->getComponent(e.target->getType(), e.target->getName()) == e.target){
                ofAbstractParameter &p = parameterGroups[i].get(e.target->getName());
                if(p.type() == typeid(ofParameter<float>).name()){
                    bool erasedConnection = false;
                    if(ofGetKeyPressed(OF_KEY_SHIFT)){
                        for(int i = 0 ; i < midiFloatConnections.size(); i++){
                            if(midiFloatConnections[i].getParameter() == &p){
                                erasedConnection = true;
                                midiFloatConnections.erase(midiFloatConnections.begin()+i);
                                return;
                            }
                        }
                    }
                    if(!erasedConnection)
                        midiFloatConnections.push_back(midiConnection<float>(&p.cast<float>()));
                }
                else if(p.type() == typeid(ofParameter<int>).name()){
                    bool erasedConnection = false;
                    if(ofGetKeyPressed(OF_KEY_SHIFT)){
                        for(int i = 0 ; i < midiIntConnections.size(
                            ); i++){
                            if(midiIntConnections[i].getParameter() == &p){
                                erasedConnection = true;
                                midiIntConnections.erase(midiIntConnections.begin()+i);
                                return;
                            }
                        }
                    }
                    if(!erasedConnection)
                    midiIntConnections.push_back(midiConnection<int>(&p.cast<int>()));
                }
                else if(p.type() == typeid(ofParameter<bool>).name()){
                    bool erasedConnection = false;
                    if(ofGetKeyPressed(OF_KEY_SHIFT)){
                        for(int i = 0 ; i < midiBoolConnections.size(); i++){
                            if(midiBoolConnections[i].getParameter() == &p){
                                erasedConnection = true;
                                midiBoolConnections.erase(midiBoolConnections.begin()+i);
                                return;
                            }
                        }
                    }
                    if(!erasedConnection)
                    midiBoolConnections.push_back(midiConnection<bool>(&p.cast<bool>()));
                }
                else if(p.type() == typeid(ofParameterGroup).name()){
                    bool erasedConnection = false;
                    if(ofGetKeyPressed(OF_KEY_SHIFT)){
                        for(int i = 0 ; i < midiIntConnections.size(); i++){
                            if(midiIntConnections[i].getParameter() == &parameterGroups[i].getGroup(e.target->getName()).getInt(1)){
                                erasedConnection = true;
                                midiIntConnections.erase(midiIntConnections.begin()+i);
                                return;
                            }
                        }
                    }
                    if(!erasedConnection)
                    midiIntConnections.push_back(midiConnection<int>(&parameterGroups[i].getGroup(e.target->getName()).getInt(1)));
                }
                else
                    ofLog() << "Cannot midi to parameter " << p.getName();
            }
        }
    }
//        else{
//        for (int i=0; i < datGuis.size() ; i++){
//            if(datGuis[i]->getComponent(e.target->getType(), e.target->getName()) == e.target)
//                connections.back().connectTo(e.target, &parameterGroups[i].get(e.target->getName()));
//        }
//    }
}

#pragma mark --Keyboard and mouse Listenerrs--

void parametersControl::keyPressed(ofKeyEventArgs &e){

}

void parametersControl::keyReleased(ofKeyEventArgs &e){

}

void parametersControl::mouseMoved(ofMouseEventArgs &e){
    
}

void parametersControl::mouseDragged(ofMouseEventArgs &e){
    if(e.button == 2 && connections.size() > 0){
        if(!connections.back().closedLine)
            connections.back().moveLine(e);
    }
}

void parametersControl::mousePressed(ofMouseEventArgs &e){
    
}

void parametersControl::mouseReleased(ofMouseEventArgs &e){
    //    if(e.button == 0 && connections.size() > 0){
    //        for(auto connection : connections){
    //            //connection.getPolyline();
    //            //connection.hitTest(e);
    //        }
    //    }
    
    if(e.button == 2 && connections.size() > 0){
        if(!connections.back().closedLine)
            connections.pop_back();
    }
}

void parametersControl::mouseScrolled(ofMouseEventArgs &e){
    
}

void parametersControl::mouseEntered(ofMouseEventArgs &e){
    
}

void parametersControl::mouseExited(ofMouseEventArgs &e){
    
}

#pragma mark ----Parameter Listerners----

void parametersControl::bpmChangedListener(float &bpm){
    newBpm = bpm;
}

void parametersControl::listenerFunction(ofAbstractParameter& e){
    int position = 0;
    nodeConnection* validConnection = nullptr;
    
    
    if(e.getName() == "Phasor Monitor"){
        return;
    }
    
    
    auto parent = e.getGroupHierarchyNames()[0];
    int parentIndex = 0;
    ofStringReplace(parent, "_", " ");
    for(int i = 0; i<parameterGroups.size(); i++){
        if(parameterGroups[i].getName() == parent)
            parentIndex = i;
    }
    
    //ParameterBinding
    for(auto &connection : connections){
        ofAbstractParameter* possibleSource = connection.getSourceParameter();
        if(possibleSource == &e && connection.closedLine){
            validConnection = &connection;
        }
    }
    
    //Midi and to gui
    int toMidiVal = 0;
    float normalizedVal = 0;
    if(e.type() == typeid(ofParameter<float>).name()){
        ofParameter<float> castedParam = e.cast<float>();
        normalizedVal = ofMap(castedParam, castedParam.getMin(), castedParam.getMax(), 0, 1);

        midiConnection<float>* validMidiConnection = nullptr;
        for(auto &midiConnection : midiFloatConnections){
            ofAbstractParameter* possibleSource = midiConnection.getParameter();
            if(possibleSource == &e && !midiConnection.isListening()){
                validMidiConnection = &midiConnection;
            }
        }
        
        if(castedParam.getName() == "BPM")
            periodTime = presetChangeBeatsPeriod / castedParam * 60.;
        
        if(validConnection != nullptr)
            setFromNormalizedValue(validConnection->getSinkParameter(), normalizedVal);
        
        if(validMidiConnection != nullptr){
            for(auto &mOut : midiOut){
                if(mOut.getName() == validMidiConnection->getDevice())
                    mOut.sendControlChange(validMidiConnection->getChannel(), validMidiConnection->getControl(), validMidiConnection->sendValue());
            }
        }
    }
    else if(e.type() == typeid(ofParameter<int>).name()){
        ofParameter<int> castedParam = e.cast<int>();
        normalizedVal = ofMap(castedParam, castedParam.getMin(), castedParam.getMax(), 0, 1);

        midiConnection<int>* validMidiConnection = nullptr;
        for(auto &midiConnection : midiIntConnections){
            ofAbstractParameter* possibleSource = midiConnection.getParameter();
            if(possibleSource == &e && !midiConnection.isListening()){
                validMidiConnection = &midiConnection;
            }
        }

        
        if(ofStringTimesInString(castedParam.getName(), "Select") == 1){
            datGuis[parentIndex]->getDropdown(castedParam.getName())->select(castedParam);
        }
        
        if(validConnection != nullptr)
            setFromNormalizedValue(validConnection->getSinkParameter(), normalizedVal);
        
        if(validMidiConnection != nullptr){
            for(auto &mOut : midiOut){
                if(mOut.getName() == validMidiConnection->getDevice())
                    mOut.sendControlChange(validMidiConnection->getChannel(), validMidiConnection->getControl(), validMidiConnection->sendValue());
            }
        }
    }
    else if(e.type() == typeid(ofParameter<bool>).name()){
        ofParameter<bool> castedParam = e.cast<bool>();
        normalizedVal = castedParam ? 1 : 0;
        
        midiConnection<bool>* validMidiConnection = nullptr;
        for(auto &midiConnection : midiBoolConnections){
            ofAbstractParameter* possibleSource = midiConnection.getParameter();
            if(possibleSource == &e && !midiConnection.isListening()){
                validMidiConnection = &midiConnection;
            }
        }
        
        //Update to datGuis
        datGuis[parentIndex]->getToggle(castedParam.getName())->setChecked(normalizedVal);
        
        if(validConnection != nullptr)
            setFromNormalizedValue(validConnection->getSinkParameter(), normalizedVal);
        
        if(validMidiConnection != nullptr){
            for(auto &mOut : midiOut){
                if(mOut.getName() == validMidiConnection->getDevice())
                    mOut.sendControlChange(validMidiConnection->getChannel(), validMidiConnection->getControl(), validMidiConnection->sendValue());
            }
        }
    }
    else if(e.type() == typeid(ofParameter<string>).name()){
        ofParameter<string> castedParam = e.cast<string>();
        position = -1;
        
        datGuis[parentIndex]->getTextInput(castedParam.getName())->setTextWithoutEvent(castedParam);
        
        if(validConnection != nullptr)
            setFromNormalizedValue(validConnection->getSinkParameter(), normalizedVal);
    }
    else if(e.type() == typeid(ofParameter<ofColor>).name()){
        ofParameter<ofColor> castedParam = e.cast<ofColor>();
        position = -1;
        
        datGuis[parentIndex]->getColorPicker(castedParam.getName())->setColor(castedParam);
    }else if(e.type() == typeid(ofParameterGroup).name()){
        ofParameter<int> castedParam = parameterGroups[parentIndex].getGroup(e.getName()).getInt(1);
        datGuis[parentIndex]->getDropdown(e.getName())->select(castedParam);
        
        midiConnection<int>* validMidiConnection = nullptr;
        for(auto &midiConnection : midiIntConnections){
            ofAbstractParameter* possibleSource = midiConnection.getParameter();
            if(possibleSource == &e && !midiConnection.isListening()){
                validMidiConnection = &midiConnection;
            }
        }
        
        if(validMidiConnection != nullptr){
            for(auto &mOut : midiOut){
                if(mOut.getName() == validMidiConnection->getDevice())
                    mOut.sendControlChange(validMidiConnection->getChannel(), validMidiConnection->getControl(), validMidiConnection->sendValue());
            }
        }
        
    }else{
        if(validConnection != nullptr)
            setFromSameTypeValue(validConnection->getSourceParameter(), validConnection->getSinkParameter());
    }

//    cout<<"Para Change: "<< e.getName() << " |pos: " << position << " |val: " << e  << " |MIDI: " << normalizedVal << endl;
}

void parametersControl::newMidiMessage(ofxMidiMessage &eventArgs){
    //Save all midi messages into a que;
    midiMessagesFiller.push_back(eventArgs);
    newMessages++;
}

void parametersControl::setFromNormalizedValue(ofAbstractParameter* e, float v){
    if(e->type() == typeid(ofParameter<float>).name()){
        ofParameter<float> castedParam = e->cast<float>();
        castedParam.set(ofMap(v, 0, 1, castedParam.getMin(), castedParam.getMax()));
    }
    else if(e->type() == typeid(ofParameter<int>).name()){
        ofParameter<int> castedParam = e->cast<int>();
        castedParam.set(ofMap(v, 0, 1, castedParam.getMin(), castedParam.getMax()));
    }
    else if(e->type() == typeid(ofParameter<bool>).name()){
        ofParameter<bool> castedParam = e->cast<bool>();
        castedParam = v<64 ? true : false;
    }
    else if(e->type() == typeid(ofParameter<ofColor>).name()){
        ofParameter<ofColor> castedParam = e->cast<ofColor>();
        castedParam.set(ofColor::fromHsb(v*255, 255, 255));
    }
    else if(e->type() == typeid(ofParameterGroup).name()){
        ofParameterGroup adoptiveGroup;
        adoptiveGroup.add(*e);
        ofParameterGroup nestedGroup = adoptiveGroup.getGroup(0);
        ofParameter<int> castedParam = nestedGroup.getInt(1);
        castedParam.set(ofMap(v, 0, 1, castedParam.getMin(), castedParam.getMax()));
    }
}

void parametersControl::setFromSameTypeValue(ofAbstractParameter* source, ofAbstractParameter* sink){
    if(source->type() == sink->type()){
        
    }else{
        int i = 0;
        for(auto &connection : connections){
            if(connection.getSourceParameter() == source && connection.getSinkParameter() == sink){
                connections.erase(connections.begin() + i);
                break;
            }
            i++;
        }
    }
        
}