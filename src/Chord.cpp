#include <Chord.h>
#include <cfloat>

#ifdef NATIVE_TEST
    #define ESP_LOGD(tag, fmt, ...) ((void)0)
#else
    #include <esp_log.h>
#endif

static const char* LOG_TAG = "Chord";

Chord::Chord()
: Chord(C,0,0,3) {}

Chord::Chord(const Chord *original)
: Chord(original->root, original->option, original->inversion, original->octave) {
    bass = original->bass;
}

Chord::Chord(uint8_t root, uint16_t option)
: Chord(root, option, 0, 3) {}

Chord::Chord(uint8_t root, uint16_t option, uint8_t inversion)
: Chord(root, option, inversion, 3) {}

Chord::Chord(uint8_t root, uint16_t option, uint8_t inversion, uint8_t octave)
: root(root), option(option), inversion(inversion), octave(octave), bass(BASS_DEFAULT) {}

std::string Chord::toString() {
    std::string str = rootStrings[root];
    str += formatChordOptions(option);

    // Add slash chord notation if a bass note is specified
    if (bass != BASS_DEFAULT) {
        str += "/" + rootStrings[bass];
    }

    return str;
}

std::string Chord::formatChordOptions(uint16_t option) {
    std::string str = "";

    // 3度
    if(option & Sus4) {
        str += "sus4";
    } else if(option & Sus2) {
        str += "sus2";
    } else if(option & Dimish) {
        str += "dim";
    } else if(option & Aug) {
        str += "aug";
    } else if(option & Minor) {
        str += "m";
    } // Major is implied

    // 7度
    if(option & MajorSeventh) {
        str += "M7";
    } else if(option & Seventh) {
        str += "7";
    } else if(option & Sixth) {
        str += "6";
    }

    // 5度
    bool fifthFlat = (option & FifthFlat) && !(option & Dimish);

    // 括弧の中に含める文字
    bool hasExtensions = false;
    std::string extensions = "";
    
    // フラットファイブとテンションが同時にある場合、フラットファイブは括弧の中に含める
    if(fifthFlat && (option & 0b1111111000000000)) {
        extensions += "♭5";
        hasExtensions = true;
    }
    
    // テンション
    if(option & Ninth || option & NinthSharp) {
        if(hasExtensions) extensions += ",";
        if(option & NinthSharp) {
            extensions += "♯9";
        } else {
            extensions += "9";
        }
        hasExtensions = true;
    }
    
    if(option & Eleventh || option & EleventhSharp) {
        if(hasExtensions) extensions += ",";
        if(option & EleventhSharp) {
            extensions += "♯11";
        } else {
            extensions += "11";
        }
        hasExtensions = true;
    }
    
    if(option & Thirteenth || option & ThirteenthSharp || option & ThirteenthFlat) {
        if(hasExtensions) extensions += ",";
        if(option & ThirteenthSharp) {
            extensions += "♯13";
        } else if(option & ThirteenthFlat) {
            extensions += "♭13";
        } else {
            extensions += "13";
        }
        hasExtensions = true;
    }
    
    // 算出した文字列の合成
    if(hasExtensions) {
        str += "(" + extensions + ")";
    } else if(fifthFlat) {
        // フラットファイブは括弧の中に含めない
        str += "-5";
    }
    
    return str;
}

std::vector<uint8_t> Chord::toMidiNoteNumbers() {
    uint8_t baseNoteNo = (octave * 12) + root;

    std::vector<uint8_t> notes = std::vector<uint8_t>();

    //Root
    notes.push_back(baseNoteNo);

    //Third
    if(option & Sus4) notes.push_back(baseNoteNo + 5);        // Perfect 4th
    else if(option & Sus2) notes.push_back(baseNoteNo + 2);   // Major 2nd
    else if((option & Minor) || (option & Dimish)) notes.push_back(baseNoteNo + 3); // Minor 3rd
    else notes.push_back(baseNoteNo + 4); //Major 3rd

    //Fifth
    if((option & FifthFlat) || (option & Dimish)) notes.push_back(baseNoteNo + 6); // Diminished 5th
    else if(option & Aug) notes.push_back(baseNoteNo + 8);    // Augmented 5th
    else notes.push_back(baseNoteNo + 7);                     // Perfect 5th

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
    float previousScore[2] = {FLT_MAX, FLT_MAX}; // 0: 1つ前, 1: 2つ前
    // Serial.printf("\ncalcInversion 0 %f\n", previousScore);
    for(uint8_t o = 3; o < 6; o++) {
        octave = o;
        for(uint8_t i = 0; i < notes.size(); i++) {
            inversion = i;
            float score = getScore(centerNoteNo);
            // Serial.printf("calcInversion %d %d %f\n", o, i, score);
            if(previousScore[1] < 8 && previousScore[1] < previousScore[0] && previousScore[1] < score) {
                // スコアが2回連続で増加してしまったので、2つ前の転回で確定
                if(inversion == 0) {
                    octave--;
                    inversion = notes.size() - 2;
                }
                else if(inversion == 1) {
                    octave--;
                    inversion = notes.size() - 1;
                }
                else inversion -= 2;
                ESP_LOGD(LOG_TAG, "calcInversion %d %d %f", octave, inversion, previousScore[1]);
                return;
            }
            previousScore[1] = previousScore[0];
            previousScore[0] = score;
        }
    }
}

// コードのボイシングを評価する
// 評価値が低いほど良いボイシング
float Chord::getScore(uint8_t centerNoteNo) {
    std::vector<uint8_t> notes = toMidiNoteNumbers();
    uint8_t bottom = notes.size() > inversion ? notes[inversion] : notes[0];
    uint8_t top = inversion == 0 ? notes.back() : (notes.size() >= inversion ? notes[inversion - 1] : notes.back());

    // 構成音の中央がcenterNoteNoと近いほど低スコア
    float mid = (bottom + top) / 2.0f;
    float result = std::abs(centerNoteNo - mid);

    // あまり音が高いと目立つので、音が高いと僅かに加点
    if(octave >= 5) {
        result += (octave - 4) * 1.0f;
        result += inversion * 0.1f;
    }

    // ルートとM7が短2度の関係にあったら加点
    if (notes.size() >= 4 && notes[0] - notes[3] == 1) {
        result += 5;
    }

    return result;
}

void Chord::setBass(int8_t bassNote) {
    // Validate the bass note is within the chromatic scale (0-11) or BASS_DEFAULT
    if (bassNote == BASS_DEFAULT || (bassNote >= 0 && bassNote <= 11)) {
        bass = bassNote;
    }
}

const std::vector<std::string> Chord::rootStrings = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

DegreeChord::DegreeChord()
: DegreeChord(I,0,0) {}

DegreeChord::DegreeChord(uint8_t root, uint16_t option)
: DegreeChord(root,option,0) {}

DegreeChord::DegreeChord(uint8_t root, uint16_t option, uint8_t inversion)
: root(root), option(option), inversion(inversion), bass(BASS_DEFAULT) {}

std::string DegreeChord::toString() {
    std::string str = rootStrings[root];
    str += Chord::formatChordOptions(option);

    // Add slash chord notation if a bass note is specified
    if (bass != BASS_DEFAULT) {
        str += "/" + rootStrings[bass];
    }

    return str;
}

void DegreeChord::setBass(int8_t bassNote) {
    // Validate the bass note is within the valid range (0-11) or BASS_DEFAULT
    if (bassNote == BASS_DEFAULT || (bassNote >= 0 && bassNote <= 11)) {
        bass = bassNote;
    }
}

bool DegreeChord::equals(DegreeChord other){
    if(other.root != this->root) return false;
    if(other.option != this->option) return false;
    return true;
}

const std::vector<std::string> DegreeChord::rootStrings = {"I","I#","II","II#","III","IV","IV#","V","V#","VI","VI#","VII"};
