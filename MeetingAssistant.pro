QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Windows specific
win32 {
    LIBS += -lole32 -loleaut32 -lmmdevapi
}

# 添加调试信息
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF /OPT:ICF

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/audioprocessor.cpp \
    src/azurespeechapi.cpp \
    src/logger.cpp \
    src/wasapiaudiocapture.cpp

HEADERS += \
    src/mainwindow.h \
    src/audioprocessor.h \
    src/azurespeechapi.h \
    src/logger.h \
    src/wasapiaudiocapture.h

FORMS += \
    src/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 添加Qt库路径
INCLUDEPATH += $$[QT_INSTALL_HEADERS]
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtCore
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtGui
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtWidgets
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtNetwork
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtMultimedia

# 添加项目源文件目录
INCLUDEPATH += src

# Azure Speech SDK (本地路径)
INCLUDEPATH += third_party/azure_speech_sdk/include/cxx_api
INCLUDEPATH += third_party/azure_speech_sdk/include/c_api

# 添加库路径
LIBS += -L$$PWD/third_party/azure_speech_sdk/lib \
        -lMicrosoft.CognitiveServices.Speech.core

# 复制运行时依赖
win32 {
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.core.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.core.dll\") && \
    $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.extension.audio.sys.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.extension.audio.sys.dll\") && \
    $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.extension.codec.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.extension.codec.dll\") && \
    $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.extension.kws.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.extension.kws.dll\") && \
    $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.extension.kws.ort.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.extension.kws.ort.dll\") && \
    $$quote(cmd /c copy /Y \"$$PWD\\third_party\\azure_speech_sdk\\bin\\Microsoft.CognitiveServices.Speech.extension.lu.dll\" \"$$OUT_PWD\\release\\Microsoft.CognitiveServices.Speech.extension.lu.dll\")
}

# 添加 Windows 调试帮助库
LIBS += -ldbghelp 