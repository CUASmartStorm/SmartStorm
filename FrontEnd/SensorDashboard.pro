QT += core gui widgets charts network

CONFIG += c++17

TARGET = SensorDashboard

# ── Layout mode ─────────────────────────────────────────────────
# Uncomment the line below to enable scrollable charts
#   (each chart gets a fixed height; user scrolls to see all)
# Leave commented for fit-on-screen mode
#   (all charts share the available space equally)
#DEFINES += SCROLLABLE_CHARTS

SOURCES += \
    main.cpp \
    SensorDashboard.cpp

HEADERS += \
    SensorDashboard.h

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
