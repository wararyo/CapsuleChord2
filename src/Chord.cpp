#include <Chord.h>

Chord::Chord()
: Chord(C,0,0,3) {}

Chord::Chord(uint8_t root, uint16_t option)
: Chord(root,option,0,3) {}

Chord::Chord(uint8_t root, uint16_t option, uint8_t inversion)
: Chord(root,option,inversion,3) {}

Chord::Chord(uint8_t root, uint16_t option, uint8_t inversion, uint8_t octave)
: root(root) , option(option), inversion(inversion), octave(octave) {}

String Chord::toString() {
    String str = "";
    str += rootStrings[root];
    for(auto item : optionStrings) {
        if(option & item.first) {
            str += item.second;
        }
    }
    return str;
}

std::vector<uint8_t> Chord::toMidiNoteNumbers() {
    uint8_t baseNoteNo = (octave * 12) + root;

    std::vector<uint8_t> notes = std::vector<uint8_t>();

    //Root
    notes.push_back(baseNoteNo);

    //Third
    if(option & Sus4) notes.push_back(baseNoteNo + 5);
    else if(option & Minor) notes.push_back(baseNoteNo + 3);
    else notes.push_back(baseNoteNo + 4); //Major

    //Fifth
    if(option & FifthFlat) notes.push_back(baseNoteNo + 6);
    else if(option & Aug) notes.push_back(baseNoteNo + 8);
    else notes.push_back(baseNoteNo + 7);

    //Thirteenth
    if(option & Thirteenth) notes.push_back(baseNoteNo + 9);

    //Seventh
    if(option & Seventh) notes.push_back(baseNoteNo + 10);
    else if(option & MajorSeventh) notes.push_back(baseNoteNo + 11);

    //Ninth
    if(option & Ninth) notes.push_back(baseNoteNo + 14);

    //Eleventh
    if(option & Eleventh) notes.push_back(baseNoteNo + 17);

    // 転回
    for(uint8_t i = 0;i < inversion;i++) {
        if(notes.size() > i) notes[i] += 12;
    }

    return notes;
}

void Chord::calcInversion(uint8_t centerNoteNo) {
    inversion = 0;
    octave = 3;
    std::vector<uint8_t> notes = toMidiNoteNumbers();
    float previousScore = getScore(centerNoteNo);
    Serial.printf("\ncalcInversion 0 %f\n", previousScore);
    for(uint8_t o = 3; o < 6; o++) {
        octave = o;
        for(uint8_t i = 1; i <= notes.size(); i++) {
            inversion = i;
            float score = getScore(centerNoteNo);
            Serial.printf("calcInversion %d %f\n", i, score);
            if(score > previousScore) {
                if(inversion == 0) {
                    octave--;
                    inversion = notes.size();
                }
                else inversion--;
                return;
            }
            previousScore = score;
        }
    }
}

float Chord::getScore(uint8_t centerNoteNo) {
    std::vector<uint8_t> notes = toMidiNoteNumbers();
    uint8_t bottom = notes.size() > inversion ? notes[inversion] : notes[0];
    uint8_t top = inversion == 0 ? notes.back() : (notes.size() >= inversion ? notes[inversion - 1] : notes.back());

    // 構成音の中央がcenterNoteNoと近いほど低スコア
    float mid = (bottom + top) / 2.0f;
    float result = std::abs(centerNoteNo - mid);

    // TODO: 短2度の音程を含むと高スコア

    return result;
}

const std::vector<String> Chord::rootStrings = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
const std::map<uint16_t,String> Chord::optionStrings = {
        {Major  , ""},
        {Minor  , "m"},
        {Dimish , "dim"},
        {Sus4   , "sus4"},
        {Sus2   , "sus2"},
        {Aug    , "aug"},
        {Seventh, "7"},
        {MajorSeventh, "M7"},
        {Sixth, "6"},
        {FifthFlat       , "-5"},
        {Ninth           , "9"},
        {NinthSharp      , "♯9"},
        {Eleventh        , "11"},
        {EleventhSharp   , "♯11"},
        {Thirteenth      , "13"},
        {ThirteenthSharp , "♯13"},
        {ThirteenthFlat  , "♭13"},
    };

DegreeChord::DegreeChord()
: DegreeChord(I,0,0) {}

DegreeChord::DegreeChord(uint8_t root, uint16_t option)
: DegreeChord(root,option,0) {}

DegreeChord::DegreeChord(uint8_t root, uint16_t option, uint8_t inversion)
: root(root) , option(option), inversion(inversion) {}

String DegreeChord::toString() {
    String str = "";
    str += rootStrings[root];
    for(auto item : Chord::optionStrings) {
        if(option & item.first) {
            str += item.second;
        }
    }
    return str;
}

bool DegreeChord::equals(DegreeChord other){
    if(other.root != this->root) return false;
    if(other.option != this->option) return false;
    return true;
}

const std::vector<String> DegreeChord::rootStrings = {"I","I#","II","II#","III","IV","IV#","V","V#","VI","VI#","VII"};
