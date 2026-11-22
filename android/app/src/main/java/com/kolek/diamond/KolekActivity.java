package com.kolek.diamond;

import android.view.KeyEvent;

import org.libsdl.app.SDLActivity;

/**
 * Activity игры. Системная кнопка «Назад» (в том числе жестовая навигация)
 * превращается в обычное нажатие KEYCODE_BACK: игра обрабатывает его как
 * экранную кнопку «МЕНЮ».
 */
public class KolekActivity extends SDLActivity {
    @Override
    public void onBackPressed() {
        if (isFinishing()) {
            return;
        }
        SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_BACK);
        // Отпускаем клавишу чуть позже, иначе SDL не успеет увидеть нажатие
        // между двумя кадрами опроса клавиатуры.
        getWindow().getDecorView().postDelayed(new Runnable() {
            @Override
            public void run() {
                SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_BACK);
            }
        }, 80);
    }
}
