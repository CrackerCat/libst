APP_ABI := all

#ʹ��Ĭ����С��C++���п⣬�������ɵ�Ӧ�����С���ڴ�ռ��С�������ֹ��ܽ��޷�֧��
#APP_STL := system

#ʹ�� GNU libstdc++ ��Ϊ��̬��,����ʹ���쳣
#APP_STL := gnustl_static

#ʹ��STLport��Ϊ��̬�⣬������Android�����������Ƽ���,��֧���쳣��RTTI
#APP_STL := stlport_static

#STLport ��Ϊ��̬�⣬������ܲ��������ԺͲ��ֵͰ汾��Android�̼���Ŀǰ���Ƽ�ʹ��
#APP_STL := stlport_shared

#APP_STL := c++_shared
APP_STL := c++_static

APP_CPPFLAGS := -D__GXX_EXPERIMENTAL_CXX0X__ -std=c++1y -frtti -fvisibility=hidden -fvisibility-inlines-hidden -fexceptions -Wno-error=format-security -fsigned-char -Os $(CPPFLAGS)

PP_DEBUG := $(strip $(NDK_DEBUG))
ifeq ($(APP_DEBUG),1)
  APP_OPTIM := debug
else
  APP_OPTIM := release
endif
