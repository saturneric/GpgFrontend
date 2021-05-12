aux_source_directory(. QTUI_SOURCE)

add_library(qtui STATIC ${QTUI_SOURCE})

qt5_use_modules(qtui Network PrintSupport Widgets Test Core)