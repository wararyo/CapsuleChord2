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
    // ダイアトニックコードを度数表現のまま返す
    virtual DegreeChord getDiatonicDegree(uint8_t number, bool seventh){return DegreeChord(number,seventh?Chord::Seventh:0);}
    Chord getDiatonic(uint8_t key, uint8_t number, bool seventh){return degreeToChord(key,getDiatonicDegree(number,seventh));}
    // DegreeChordを実際に鳴らせるChordに具現化する（degreeToChord + calcInversion）
    Chord realizeChord(uint8_t key, DegreeChord degree, uint8_t centerNoteNo);
};

class MajorScale : public ScaleBase {
public:
    static const uint16_t diatonicOptions[];
    static const uint16_t diatonicSeventhOptions[];
    std::string name() override {return "Major";};
    static const uint8_t pitch[];
    DegreeChord getDiatonicDegree(uint8_t number, bool seventh) override;
};

class MinorScale : public ScaleBase {
public:
    static const uint16_t diatonicOptions[];
    static const uint16_t diatonicSeventhOptions[];
    std::string name() override {return "Minor";};
    static const uint8_t pitch[];
    DegreeChord getDiatonicDegree(uint8_t number, bool seventh) override;
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
    DegreeChord getDiatonicDegree(uint8_t number, bool seventh);
    Chord getDiatonic(uint8_t number, bool seventh);
    Chord realizeChord(DegreeChord degree, uint8_t centerNoteNo);
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