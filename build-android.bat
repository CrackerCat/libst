@echo off

REM ��Ϊʹ����C++14��ԭ��NDK���GCC�汾����������4.9�����²�֧��C++14. ֻ���л���clang��
REM ���޸ġ�NDK_TOOLCHAIN_VERSION=clang3.6����ʹ�ñ���NDK����ڵ�clang�汾
"%ANDROID_NDK_ROOT%\ndk-build" NDK_TOOLCHAIN_VERSION=clang3.6 -B NDK_DEBUG=0 NDK_PROJECT_PATH=%~dp0
