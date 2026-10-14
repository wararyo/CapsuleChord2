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
    // 度数root（半音表現 0-11）がスケール構成音ならダイアトニック番号(0-6)を、そうでなければ-1を返す
    virtual int getDiatonicNumber(uint8_t degreeRoot){(void)degreeRoot; return -1;}
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
    int getDiatonicNumber(uint8_t degreeRoot) override;
};

class MinorScale : public ScaleBase {
public:
    static const uint16_t diatonicOptions[];
    static const uint16_t diatonicSeventhOptions[];
    std::string name() override {return "Minor";};
    static const uint8_t pitch[];
    DegreeChord getDiatonicDegree(uint8_t number, bool seventh) override;
    int getDiatonicNumber(uint8_t degreeRoot) override;
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
    int getDiatonicNumber(uint8_t degreeRoot);
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
        // ネストが無い・空の場合はpopNestと対にならないため何もせず戻る（現在値を保持）
        if (!archive.pushNest(keyName)) return;
        // toString()がChord::rootStrings[key]を参照するため範囲外の値は採用しない
        int rawKey = key;
        archive("Key", rawKey);
        if (rawKey >= 0 && rawKey < 12) key = static_cast<uint8_t>(rawKey);
        // Find scale which has its name
        std::string scaleStr = "";
        archive("Scale", scaleStr);
        currentScale = getScaleFromName(scaleStr);
        archive.popNest();
    }
};

#endif