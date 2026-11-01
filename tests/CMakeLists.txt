add_executable(ReaperLooperTests
    SmokeTests.cpp
    engine/LaunchQuantizerTests.cpp
    engine/LoopSegmenterTests.cpp
    model/MidiClipTests.cpp
    session/TrackTests.cpp
    session/SessionTests.cpp
)

target_include_directories(ReaperLooperTests PRIVATE ${CMAKE_SOURCE_DIR}/source)

target_link_libraries(ReaperLooperTests
    PRIVATE
        ReaperLooperCore
        Catch2::Catch2WithMain
)

catch_discover_tests(ReaperLooperTests)
