rmdir "app\src\main\jni\SDL2"
rmdir "app\src\main\jni\SDL2_image"
mklink /d "app\src\main\jni\SDL2" "C:\AndroidLIB\SDL2-2.0.8"
mklink /d "app\src\main\jni\SDL2_image" "C:\AndroidLIB\SDL2_image-2.0.2"
echo Done.
pause