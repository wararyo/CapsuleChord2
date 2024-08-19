#pragma once

#include <lvgl.h>

// アプリの基底クラス
class AppBase
{
public:
    // アプリ名
    virtual char *getAppName() { return ""; }

    // バックグラウンドで動作するかどうか
    // Tempo等のコールバックをバックグラウンドでフックするアプリはtrueを返す必要がある
    virtual bool runsInBackground() { return false; }

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
};
