# Android-сборка «Апартаментов ДАЙМОНД»

Полные требования и инструкции — в корневом [README.md](../README.md).
Коротко:

```bash
# Ubuntu / macOS
export ANDROID_HOME="$HOME/Android/Sdk"
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

```powershell
# Windows PowerShell
$env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
.\gradlew.bat assembleDebug
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

Gradle сам скачает SDL2 2.32.10 при первой сборке. Для офлайн-сборки:

- Ubuntu: `./fetch-sdl2.sh`
- Windows: `powershell -ExecutionPolicy Bypass -File .\fetch-sdl2.ps1`

Исходники SDL2 окажутся в `android/SDL2` и подхватятся автоматически.

APK: `app/build/outputs/apk/debug/app-debug.apk`.
