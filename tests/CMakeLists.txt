add_executable(test_arm test_arm.c test_common.h)
add_executable(test_thumb test_thumb.c test_common.h)
target_link_libraries(test_arm common arm7tdmi core audio render)
target_link_libraries(test_thumb common arm7tdmi core audio render)
add_test(test_arm test_arm)
add_test(test_thumb test_thumb)
configure_file(gba-suite/arm.gba arm.gba COPYONLY)
configure_file(gba-suite/arm.log arm.log COPYONLY)
configure_file(gba-suite/thumb.gba thumb.gba COPYONLY)
configure_file(gba-suite/thumb.log thumb.log COPYONLY)
