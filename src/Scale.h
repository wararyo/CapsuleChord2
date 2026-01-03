#ifndef _SCALE_H_
#define _SCALE_H_

#include <stdint.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>
#include "Chord.h"
#include "Archive.h"

//ある特定の種類のスケールを定義するための基底クラス
class ScaleBase {
public:
    virtual std::string name() {return "BaseScale";};
    virtual Chord degreeToChord(uint8_t key, DegreeChord degree);
    virtual Chord getDiatonic(uint8_t key, uint8_t number, bool seventh, Chord base = Chord()){return degreeToChord(key,DegreeChord(number,seventh?Chord::Seventh:0));}
};

class MajorScale : public ScaleBase {
public:
    static const uint16_t diatonicOptions[];
    static const uint16_t diatonicSeventhOptions[];
    std::string name() override {return "Major";};
    static const uint8_t pitch[];
    // Chord degreeToChord(uint8_t key, DegreeChord degree, Chord base = Chord()) override;
    Chord getDiatonic(uint8_t key, uint8_t number, bool seventh, Chord base = Chord()) override;
};

class MinorScale : public ScaleBase {
public:
    static const uint16_t diatonicOptions[];
    static const uint16_t diatonicSeventhOptions[];
    std::string name() override {return "Minor";};
    static const uint8_t pitch[];
    // Chord degreeToChord(uint8_t key, DegreeChord degree, Chord base = Chord()) override;
    Chord getDiatonic(uint8_t key, uint8_t number, bool seventh, Chord base = Chord()) override;
};

//Chordクラスと同様に現在使っているスケールを管理するためのクラス
class Scale {
protected:
    static std::vector<std::shared_ptr<ScaleBase>> availableScales;
public:
    Scale();
    Scale(uint8_t key);
    uint8_t key = 0; //主音 C=0
    ScaleBase *currentScale;

    Chord degreeToChord(DegreeChord degree);
    Chord getDiatonic(uint8_t number, bool seventh, Chord base = Chord());
    std::string toString() const;
    static std::vector<std::shared_ptr<ScaleBase>> getAvailableScales();

    // 比較演算子
    bool operator==(const Scale& other) const {
        return key == other.key && currentScale == other.currentScale;
    }
    bool operator!=(const Scale& other) const {
        return !(*this == other);
    }
    int getScaleIndex();
    int getScaleIndexFromName(const std::string& scaleStr);
    ScaleBase *getScaleFromName(const std::string& scaleStr);

    void serialize(OutputArchive &archive,const char *keyName) const {
        archive.pushNest(keyName);
        archive("Key", key);
        archive("Scale", currentScale->name());
        archive.popNest();
    }
    void deserialize(InputArchive &archive,const char *keyName) {
        archive.pushNest(keyName);
        archive("Key", key);
        // Find scale which has its name
        std::string scaleStr = "";
        archive("Scale", scaleStr);
        currentScale = getScaleFromName(scaleStr);
        archive.popNest();
    }
};

#endif