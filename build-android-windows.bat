@echo off
cls

REM ��Ϊʹ����C++14��ԭ��NDK���GCC�汾����������4.9�����²�֧��C++14. ֻ���л���clang��
REM ���޸ġ�NDK_TOOLCHAIN_VERSION=clang3.6����ʹ�ñ���NDK����ڵ�clang�汾

"%ANDROID_NDK_ROOT%\ndk-build" NDK_TOOLCHAIN_VERSION=clang3.6 -B NDK_DEBUG=0 NDK_PROJECT_PATH=%~dp0

REM mkdir .\libs\Android\armeabi
REM xcopy .\obj\local\armeabi\libvfxbase.a .\libs\Android\armeabi\ /Y

REM mkdir .\libs\Android\armeabi-v7a
REM xcopy .\obj\local\armeabi-v7a\libvfxbase.a .\libs\Android\armeabi-v7a\ /Y

REM mkdir .\libs\Android\x86
REM xcopy .\obj\local\x86\libvfxbase.a .\libs\Android\x86\ /Y

REM mkdir .\libs\Android\mips
REM xcopy .\obj\local\mips\libvfxbase.a .\libs\Android\mips\ /Y
