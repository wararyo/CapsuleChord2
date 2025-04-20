#pragma once

#include <lvgl.h>
#include "../Context.h" // Add Context include

// アプリの基底クラス
class AppBase
{
public:
    // アプリ名
    virtual char *getAppName() { return ""; }

    // アプリアイコン
    virtual lv_img_dsc_t *getIcon() { return nullptr; }

    // バックグラウンドで動作するかどうか
    // Tempo等のコールバックをバックグラウンドでフックするアプリはtrueを返す必要がある
    virtual bool runsInBackground() { return false; }

    // アプリがバックグラウンド動作を行っているか
    virtual bool getActive() = 0;

    // コンテキストを設定する
    void setContext(Context *context) { this->context = context; }

    // アプリを初めて起動したとき
    virtual void onCreate() = 0;

    // アプリのバックグラウンド動作を開始する
    // Tempo等へのフックはここで行う
    virtual void onActivate() = 0;

    // アプリのバックグラウンド動作を一時停止する
    // ここでTempo等へのフックを解除する
    virtual void onDeactivate() = 0;

    // アプリのGUIが表示される時
    // @param container アプリのUIを配置する親コンテナ
    virtual void onShowGui(lv_obj_t *container) = 0;

    // アプリのGUIが非表示になる時
    // ここでアプリのUIを削除する
    // containerは削除しないこと
    virtual void onHideGui() = 0;

    // アプリの終了処理
    virtual void onDestroy() = 0;
    
    // メインループでUIを安全に更新するための処理
    // このメソッドはタイマーコールバックやイベントリスナー内でUIを
    // 直接更新せず、フラグを設定して後でこのメソッドで安全に更新するために使う
    virtual void onUpdateGui() {}

protected:
    Context *context = nullptr; // All apps can access context through this pointer
};
