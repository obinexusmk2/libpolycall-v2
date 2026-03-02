# tests/CMakeLists.txt
add_executable(test_agha_dozie
    test_main.c
    test_pattern_recognition.c
    test_coherence_framework.c
    test_fault_tolerance.c
)

target_link_libraries(test_agha_dozie
    PRIVATE
        agha_dozie_core
        gatogi_algorithms
)

add_test(NAME agha_dozie_tests COMMAND test_agha_dozie)
