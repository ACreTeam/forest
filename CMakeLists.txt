cmake_minimum_required(VERSION 3.25)

project(forest
    LANGUAGES C CXX
    DESCRIPTION "Animal Crossing GameCube PC Port"
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# Use static MSVC runtime (/MT or /MTd) for consistency with Dawn/abseil
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Build all third-party dependencies as static libraries.
# Only foresta_rel is explicitly SHARED (the game logic REL module).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include("cmake/get_cpm.cmake")

set(CPM_aurora_SOURCE "${PROJECT_SOURCE_DIR}/../aurora")

CPMAddPackage(
  NAME aurora
  GITHUB_REPOSITORY encounter/aurora
  GIT_TAG bd20b65d59c2bd119e078ee87a8b896cc82994d2
  OPTIONS
    "AURORA_ENABLE_GX OFF"
)

# ==============================================================================
# Global settings
# ==============================================================================

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Auto-export all symbols from shared libraries (DLLs) on Windows
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Position-independent code (required for shared library targets on Linux/macOS)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# Common interface targets for shared compile settings
# ==============================================================================

# Base include directories shared by ALL targets
add_library(forest_includes INTERFACE)
# Aurora provides replacement Dolphin SDK headers (gx, pad, si, vi, mtx, types).
# Place aurora's include dir BEFORE our own so its dolphin/ headers take precedence
# over the project's stubs for the APIs aurora implements.
target_include_directories(forest_includes BEFORE INTERFACE
    ${aurora_SOURCE_DIR}/include
)
target_include_directories(forest_includes INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}    # for "assets/..." includes in bootdata/logo sources
    # Extracted asset .inc files from the original ROM (TODO: automate extraction)
    ${CMAKE_SOURCE_DIR}/build/GAFE01_00/include
)

# Common defines for game code (both DOL and REL)
add_library(forest_common INTERFACE)
target_link_libraries(forest_common INTERFACE forest_includes)
target_compile_definitions(forest_common INTERFACE
    _LANGUAGE_C
    F3DEX_GBI_2
    VERSION=0          # USA (GAFE01_00)
    _CRT_SECURE_NO_WARNINGS    # Suppress MSVC CRT deprecation warnings
    AURORA                     # Building against aurora SDK replacement
    TARGET_PC                  # PC port build
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:DEBUG=0>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)
# Silence warnings that are pervasive in decompiled code
target_compile_options(forest_common INTERFACE
    # C++17 removed 'register': allow it for GCN-era code
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<C_COMPILER_ID:MSVC>>>:-Wno-register>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<C_COMPILER_ID:MSVC>>:/wd5033>
    # Allow multi-character constants (used for FourCC tags like 'DATA')
    # (MSVC does not warn on multi-character constants by default)
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-multichar>
    # Pointer-to-int casts are pervasive in 32-bit GCN code
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-pointer-to-int-cast>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-int-to-pointer-cast>
    $<$<C_COMPILER_ID:MSVC>:/wd4302>    # 'conversion': truncation from pointer
    $<$<C_COMPILER_ID:MSVC>:/wd4311>    # pointer truncation from 'type' to 'type'
    $<$<C_COMPILER_ID:MSVC>:/wd4312>    # conversion from 'type' to 'type' of greater size
    # Decompiled code has many sign/implicit conversion patterns
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-implicit-function-declaration>
    $<$<C_COMPILER_ID:MSVC>:/wd4013>    # function undefined; assuming extern returning int
    # Narrowing conversions (e.g. -1 to u16) are common in GCN-era initializers
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<C_COMPILER_ID:MSVC>>>:-Wno-c++11-narrowing>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<C_COMPILER_ID:MSVC>>:/wd4838>
)

# Settings specific to static DOL libraries
add_library(forest_static INTERFACE)
target_link_libraries(forest_static INTERFACE forest_common)
target_compile_definitions(forest_static INTERFACE
    MUST_MATCH
)

# Settings specific to REL (foresta) libraries
add_library(forest_rel INTERFACE)
target_link_libraries(forest_rel INTERFACE forest_common)
target_compile_definitions(forest_rel INTERFACE
    IS_REL
    MUST_MATCH
)

# ==============================================================================
# Helper function: create a library target with proper settings
# ==============================================================================

function(forest_add_library target_name)
    cmake_parse_arguments(ARG
        ""                          # options
        "BASE_DIR;INTERFACE_TARGET" # one-value args
        "SOURCES;CXX_SOURCES"       # multi-value args
        ${ARGN}
    )

    # Collect all sources
    set(all_sources ${ARG_SOURCES} ${ARG_CXX_SOURCES})

    if(NOT all_sources)
        message(WARNING "forest_add_library(${target_name}): no sources specified")
        return()
    endif()

    # Prepend base directory to relative paths
    set(full_sources "")
    foreach(src IN LISTS all_sources)
        if(IS_ABSOLUTE ${src})
            list(APPEND full_sources ${src})
        else()
            list(APPEND full_sources ${ARG_BASE_DIR}/${src})
        endif()
    endforeach()

    add_library(${target_name} STATIC ${full_sources})

    # Link the interface target for shared settings
    if(ARG_INTERFACE_TARGET)
        target_link_libraries(${target_name} PUBLIC ${ARG_INTERFACE_TARGET})
    endif()

    # Mark C files that should be compiled as C++ (via CXX_SOURCES)
    if(ARG_CXX_SOURCES)
        set(full_cxx_sources "")
        foreach(src IN LISTS ARG_CXX_SOURCES)
            if(IS_ABSOLUTE ${src})
                list(APPEND full_cxx_sources ${src})
            else()
                list(APPEND full_cxx_sources ${ARG_BASE_DIR}/${src})
            endif()
        endforeach()
        set_source_files_properties(${full_cxx_sources} PROPERTIES LANGUAGE CXX)
    endif()
endfunction()

# Static libraries -- the """engine""" code
add_subdirectory(src/static)

# ==============================================================================
# Foresta REL — Game Logic Libraries
# ==============================================================================
# In the original build these form the foresta.rel relocatable module.
# For the PC port they are compiled as regular static libraries.
# ==============================================================================

# --- foresta ---
# Core game loop, main entry, graph thread, memory allocators
forest_add_library(foresta
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        audio.c
        c_keyframe.c
        ev_cherry_manager.c
        evw_anime.c
        f_furniture.c
        famicom_emu.c
        first_game.c
        game.c
        gamealloc.c
        gfxalloc.c
        graph.c
        irqmgr.c
        lb_reki.c
        lb_rtc.c
        main.c
        padmgr.c
        player_select.c
        PreRender.c
        s_cpak.c
        save_menu.c
        second_game.c
        THA_GA.c
        TwoHeadArena.c
        zurumode.c
)

# --- actor ---
# All in-game actor entities (buildings, items, weather, vehicles, etc.)
forest_add_library(actor
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        actor/ac_airplane.c
        actor/ac_animal_logo.c
        actor/ac_ant.c
        actor/ac_aprilfool_control.c
        actor/ac_arrange_ftr.c
        actor/ac_arrange_room.c
        actor/ac_ball.c
        actor/ac_balloon.c
        actor/ac_bee.c
        actor/ac_birth_control.c
        actor/ac_boat.c
        actor/ac_boat_demo.c
        actor/ac_boxManager.c
        actor/ac_boxMove.c
        actor/ac_boxTrick01.c
        actor/ac_br_shop.c
        actor/ac_bridge_a.c
        actor/ac_broker_design.c
        actor/ac_buggy.c
        actor/ac_conveni.c
        actor/ac_cottage.c
        actor/ac_count02.c
        actor/ac_countdown.c
        actor/ac_depart.c
        actor/ac_douzou.c
        actor/ac_dummy.c
        actor/ac_dump.c
        actor/ac_effectbg.c
        actor/ac_event_manager.c
        actor/ac_fallS.c
        actor/ac_fallSESW.c
        actor/ac_field_draw.c
        actor/ac_fieldm_draw.c
        actor/ac_flag.c
        actor/ac_fuusen.c
        actor/ac_garagara.c
        actor/ac_ghog.c
        actor/ac_goza.c
        actor/ac_groundhog_control.c
        actor/ac_gyo_kage.c
        actor/ac_gyo_kaseki.c
        actor/ac_gyo_release.c
        actor/ac_gyo_test.c
        actor/ac_gyoei.c
        actor/ac_handOverItem.c
        actor/ac_haniwa.c
        actor/ac_hatumode_control.c
        actor/ac_house.c
        actor/ac_house_clock.c
        actor/ac_house_goki.c
        actor/ac_htable.c
        actor/ac_ins_amenbo.c
        actor/ac_ins_batta.c
        actor/ac_ins_chou.c
        actor/ac_ins_dango.c
        actor/ac_ins_goki.c
        actor/ac_ins_hitodama.c
        actor/ac_ins_hotaru.c
        actor/ac_ins_ka.c
        actor/ac_ins_kabuto.c
        actor/ac_ins_kera.c
        actor/ac_ins_mino.c
        actor/ac_ins_semi.c
        actor/ac_ins_tentou.c
        actor/ac_ins_tonbo.c
        actor/ac_insect.c
        actor/ac_intro_demo.c
        actor/ac_kago.c
        actor/ac_kamakura.c
        actor/ac_kamakura_indoor.c
        actor/ac_koinobori.c
        actor/ac_lighthouse_switch.c
        actor/ac_lotus.c
        actor/ac_mailbox.c
        actor/ac_mbg.c
        actor/ac_mikanbox.c
        actor/ac_mikuji.c
        actor/ac_misin.c
        actor/ac_mscore_control.c
        actor/ac_mural.c
        actor/ac_museum.c
        actor/ac_museum_fish.c
        actor/ac_museum_fossil.c
        actor/ac_museum_indoor.c
        actor/ac_museum_insect.c
        actor/ac_museum_picture.c
        actor/ac_my_house.c
        actor/ac_my_indoor.c
        actor/ac_my_room.c
        actor/ac_nameplate.c
        actor/ac_needlework_indoor.c
        actor/ac_needlework_shop.c
        actor/ac_police_box.c
        actor/ac_post_office.c
        actor/ac_present_demo.c
        actor/ac_psnowman.c
        actor/ac_pterminal.c
        actor/ac_quest_manager.c
        actor/ac_quest_talk_fj_init.c
        actor/ac_quest_talk_greeting.c
        actor/ac_quest_talk_init.c
        actor/ac_quest_talk_island.c
        actor/ac_quest_talk_normal_init.c
        actor/ac_radio.c
        actor/ac_reserve.c
        actor/ac_reset_demo.c
        actor/ac_ride_off_demo.c
        actor/ac_rope.c
        actor/ac_s_car.c
        actor/ac_sample.c
        actor/ac_set_manager.c
        actor/ac_set_npc_manager.c
        actor/ac_set_ovl_gyoei.c
        actor/ac_set_ovl_insect.c
        actor/ac_shop.c
        actor/ac_shop_design.c
        actor/ac_shop_goods.c
        actor/ac_shop_indoor.c
        actor/ac_shop_level.c
        actor/ac_shop_manekin.c
        actor/ac_shop_umbrella.c
        actor/ac_shrine.c
        actor/ac_sign.c
        actor/ac_snowman.c
        actor/ac_station.c
        actor/ac_structure.c
        actor/ac_super.c
        actor/ac_tama.c
        actor/ac_tent.c
        actor/ac_tokyoso_control.c
        actor/ac_tools.c
        actor/ac_toudai.c
        actor/ac_train0.c
        actor/ac_train1.c
        actor/ac_train_door.c
        actor/ac_train_window.c
        actor/ac_tukimi.c
        actor/ac_tunahiki_control.c
        actor/ac_turi.c
        actor/ac_uki.c
        actor/ac_weather.c
        actor/ac_weather_fine.c
        actor/ac_weather_leaf.c
        actor/ac_weather_rain.c
        actor/ac_weather_sakura.c
        actor/ac_weather_snow.c
        actor/ac_windmill.c
        actor/ac_yatai.c
)

# --- actor_npc ---
# NPC actors: villagers, special NPCs, shop masters, guides, etc.
forest_add_library(actor_npc
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        actor/npc/ac_countdown_npc0.c
        actor/npc/ac_countdown_npc1.c
        actor/npc/ac_go_home_npc.c
        actor/npc/ac_groundhog_npc0.c
        actor/npc/ac_halloween_npc.c
        actor/npc/ac_hanabi_npc0.c
        actor/npc/ac_hanabi_npc1.c
        actor/npc/ac_hanami_npc0.c
        actor/npc/ac_hanami_npc1.c
        actor/npc/ac_harvest_npc0.c
        actor/npc/ac_harvest_npc1.c
        actor/npc/ac_hatumode_npc0.c
        actor/npc/ac_kamakura_npc0.c
        actor/npc/ac_normal_npc.c
        actor/npc/ac_npc.c
        actor/npc/ac_npc2.c
        actor/npc/ac_npc_conv_master.c
        actor/npc/ac_npc_curator.c
        actor/npc/ac_npc_depart_master.c
        actor/npc/ac_npc_engineer.c
        actor/npc/ac_npc_guide.c
        actor/npc/ac_npc_guide2.c
        actor/npc/ac_npc_hem.c
        actor/npc/ac_npc_majin.c
        actor/npc/ac_npc_majin2.c
        actor/npc/ac_npc_majin3.c
        actor/npc/ac_npc_majin4.c
        actor/npc/ac_npc_majin5.c
        actor/npc/ac_npc_mamedanuki.c
        actor/npc/ac_npc_mask_cat.c
        actor/npc/ac_npc_mask_cat2.c
        actor/npc/ac_npc_needlework.c
        actor/npc/ac_npc_p_sel.c
        actor/npc/ac_npc_p_sel2.c
        actor/npc/ac_npc_police.c
        actor/npc/ac_npc_police2.c
        actor/npc/ac_npc_post_girl.c
        actor/npc/ac_npc_post_man.c
        actor/npc/ac_npc_rcn_guide.c
        actor/npc/ac_npc_rcn_guide2.c
        actor/npc/ac_npc_restart.c
        actor/npc/ac_npc_rtc.c
        actor/npc/ac_npc_sendo.c
        actor/npc/ac_npc_shasho.c
        actor/npc/ac_npc_shop_master.c
        actor/npc/ac_npc_shop_mastersp.c
        actor/npc/ac_npc_sleep_obaba.c
        actor/npc/ac_npc_soncho.c
        actor/npc/ac_npc_station_master.c
        actor/npc/ac_npc_super_master.c
        actor/npc/ac_npc_totakeke.c
        actor/npc/ac_present_npc.c
        actor/npc/ac_taisou_npc0.c
        actor/npc/ac_tamaire_npc0.c
        actor/npc/ac_tamaire_npc1.c
        actor/npc/ac_tokyoso_npc0.c
        actor/npc/ac_tokyoso_npc1.c
        actor/npc/ac_tukimi_npc0.c
        actor/npc/ac_tukimi_npc1.c
        actor/npc/ac_tunahiki_npc0.c
        actor/npc/ac_tunahiki_npc1.c
        actor/npc/ac_turi_npc0.c
)

# --- actor_npc_event ---
# Event-specific NPC actors (santa, artist, ghost, gypsy, etc.)
forest_add_library(actor_npc_event
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        actor/npc/event/ac_ev_angler.c
        actor/npc/event/ac_ev_artist.c
        actor/npc/event/ac_ev_broker.c
        actor/npc/event/ac_ev_broker2.c
        actor/npc/event/ac_ev_carpetPeddler.c
        actor/npc/event/ac_ev_castaway.c
        actor/npc/event/ac_ev_designer.c
        actor/npc/event/ac_ev_dokutu.c
        actor/npc/event/ac_ev_dozaemon.c
        actor/npc/event/ac_ev_ghost.c
        actor/npc/event/ac_ev_gypsy.c
        actor/npc/event/ac_ev_kabuPeddler.c
        actor/npc/event/ac_ev_majin.c
        actor/npc/event/ac_ev_miko.c
        actor/npc/event/ac_ev_pumpkin.c
        actor/npc/event/ac_ev_santa.c
        actor/npc/event/ac_ev_soncho.c
        actor/npc/event/ac_ev_soncho2.c
        actor/npc/event/ac_ev_speech_soncho.c
        actor/npc/event/ac_ev_turkey.c
        actor/npc/event/ac_ev_yomise.c
)

# --- actor_tool ---
# Held tool actors (umbrella, flags, hats, flowers, etc.)
forest_add_library(actor_tool
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        actor/tool/ac_t_anrium1.c
        actor/tool/ac_t_bag1.c
        actor/tool/ac_t_bag2.c
        actor/tool/ac_t_biscus1.c
        actor/tool/ac_t_biscus2.c
        actor/tool/ac_t_biscus3.c
        actor/tool/ac_t_biscus4.c
        actor/tool/ac_t_cobra1.c
        actor/tool/ac_t_cracker.c
        actor/tool/ac_t_flag.c
        actor/tool/ac_t_hanabi.c
        actor/tool/ac_t_hasu1.c
        actor/tool/ac_t_hat1.c
        actor/tool/ac_t_hat2.c
        actor/tool/ac_t_hat3.c
        actor/tool/ac_t_keitai.c
        actor/tool/ac_t_npc_sao.c
        actor/tool/ac_t_pistol.c
        actor/tool/ac_t_rei1.c
        actor/tool/ac_t_rei2.c
        actor/tool/ac_t_tama.c
        actor/tool/ac_t_tumbler.c
        actor/tool/ac_t_umbrella.c
        actor/tool/ac_t_utiwa.c
        actor/tool/ac_t_zinnia1.c
        actor/tool/ac_t_zinnia2.c
)

# --- bg_item ---
# Background placed items (ground decorations, seasonal items)
forest_add_library(bg_item
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        bg_item/bg_cherry_item.c
        bg_item/bg_item.c
        bg_item/bg_police_item.c
        bg_item/bg_post_item.c
        bg_item/bg_winter_item.c
        bg_item/bg_xmas_item.c
)

# --- effect ---
# All particle and visual effects (dust, weather, sparkles, footprints, etc.)
forest_add_library(effect
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        effect/ef_ami_mizu.c
        effect/ef_anahikari.c
        effect/ef_ase.c
        effect/ef_ase2.c
        effect/ef_ase_ch.c
        effect/ef_break_axe.c
        effect/ef_bubu.c
        effect/ef_buruburu.c
        effect/ef_bush_happa.c
        effect/ef_bush_yuki.c
        effect/ef_car_blight.c
        effect/ef_car_light.c
        effect/ef_clacker.c
        effect/ef_coin.c
        effect/ef_dash_asimoto.c
        effect/ef_dig_hole.c
        effect/ef_dig_mud.c
        effect/ef_dig_scoop.c
        effect/ef_douzou_light.c
        effect/ef_doyon.c
        effect/ef_dust.c
        effect/ef_effect_control.c
        effect/ef_flash.c
        effect/ef_footprint.c
        effect/ef_furo_yuge.c
        effect/ef_gimonhu.c
        effect/ef_goki.c
        effect/ef_ha.c
        effect/ef_halloween.c
        effect/ef_halloween_smoke.c
        effect/ef_hanabi_botan1.c
        effect/ef_hanabi_botan2.c
        effect/ef_hanabi_dummy.c
        effect/ef_hanabi_hoshi.c
        effect/ef_hanabi_set.c
        effect/ef_hanabi_switch.c
        effect/ef_hanabi_yanagi.c
        effect/ef_hanabira.c
        effect/ef_hanatiri.c
        effect/ef_hirameki_den.c
        effect/ef_hirameki_hikari.c
        effect/ef_ikigire.c
        effect/ef_impact_star.c
        effect/ef_kagu_happa.c
        effect/ef_kamifubuki.c
        effect/ef_kangaeru.c
        effect/ef_kantanhu.c
        effect/ef_kasamizu.c
        effect/ef_kasamizutama.c
        effect/ef_kaze.c
        effect/ef_kaze_happa.c
        effect/ef_kigae.c
        effect/ef_kigae_light.c
        effect/ef_kikuzu.c
        effect/ef_killer.c
        effect/ef_kisha_kemuri.c
        effect/ef_konpu.c
        effect/ef_kpun.c
        effect/ef_kyousou_onpu.c
        effect/ef_lamp_light.c
        effect/ef_lovelove.c
        effect/ef_lovelove2.c
        effect/ef_lovelove_heart.c
        effect/ef_make_hem.c
        effect/ef_mizutama.c
        effect/ef_motiyuge.c
        effect/ef_muka.c
        effect/ef_naku.c
        effect/ef_namida.c
        effect/ef_neboke.c
        effect/ef_neboke_akubi.c
        effect/ef_neboke_awa.c
        effect/ef_night13_moon.c
        effect/ef_night15_moon.c
        effect/ef_ongen.c
        effect/ef_otikomi.c
        effect/ef_otosiana.c
        effect/ef_pun.c
        effect/ef_pun_sekimen.c
        effect/ef_pun_yuge.c
        effect/ef_reset_hole.c
        effect/ef_room_sunshine.c
        effect/ef_room_sunshine_minsect.c
        effect/ef_room_sunshine_museum.c
        effect/ef_room_sunshine_police.c
        effect/ef_room_sunshine_posthouse.c
        effect/ef_sandsplash.c
        effect/ef_shock.c
        effect/ef_shooting.c
        effect/ef_shooting_kira.c
        effect/ef_shooting_set.c
        effect/ef_siawase_hana.c
        effect/ef_siawase_hana_ch.c
        effect/ef_siawase_hikari.c
        effect/ef_sibuki.c
        effect/ef_situren.c
        effect/ef_slip.c
        effect/ef_slip_footprint.c
        effect/ef_soba_yuge.c
        effect/ef_steam.c
        effect/ef_string.c
        effect/ef_suisou_awa.c
        effect/ef_swing_axe.c
        effect/ef_swing_net.c
        effect/ef_swing_rod.c
        effect/ef_taberu.c
        effect/ef_takurami.c
        effect/ef_takurami_kira.c
        effect/ef_tamaire.c
        effect/ef_tape.c
        effect/ef_tent_lamp.c
        effect/ef_tumble.c
        effect/ef_tumble_bodyprint.c
        effect/ef_tumble_dust.c
        effect/ef_turi_hamon.c
        effect/ef_turi_hane0.c
        effect/ef_turi_hane1.c
        effect/ef_turi_mizu.c
        effect/ef_turi_suiteki.c
        effect/ef_turn_asimoto.c
        effect/ef_turn_footprint.c
        effect/ef_uranai.c
        effect/ef_wait_asimoto.c
        effect/ef_walk_asimoto.c
        effect/ef_warau.c
        effect/ef_yajirushi.c
        effect/ef_young_tree.c
        effect/ef_yukidama.c
        effect/ef_yukidaruma.c
        effect/ef_yukihane.c
)

# --- game ---
# Core game systems (m_ modules): actors, player, scenes, collision, events, etc.
forest_add_library(game
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        game/m_actor.c
        game/m_actor_dlftbls.c
        game/m_actor_shadow.c
        game/m_address_ovl.c
        game/m_all_grow.c
        game/m_all_grow_ovl.c
        game/m_bank_ovl.c
        game/m_banti.c
        game/m_bg_item.c
        game/m_bg_tex.c
        game/m_bgm.c
        game/m_birthday_ovl.c
        game/m_board_ovl.c
        game/m_calendar.c
        game/m_calendar_ovl.c
        game/m_camera2.c
        game/m_card.c
        game/m_catalog_ovl.c
        game/m_choice.c
        game/m_clip.c
        game/m_cockroach.c
        game/m_collision_bg.c
        game/m_collision_obj.c
        game/m_common_data.c
        game/m_controller.c
        game/m_cpak.c
        game/m_cpedit_ovl.c
        game/m_cpmail_ovl.c
        game/m_cporiginal_ovl.c
        game/m_cpwarning_ovl.c
        game/m_debug.c
        game/m_debug_display.c
        game/m_debug_hayakawa.c
        game/m_debug_mode.c
        game/m_demo.c
        game/m_design_ovl.c
        game/m_diary.c
        game/m_diary_ovl.c
        game/m_eappli.c
        game/m_editEndChk_ovl.c
        game/m_editor_ovl.c
        game/m_event.c
        game/m_event_map_npc.c
        game/m_fbdemo.c
        game/m_fbdemo_fade.c
        game/m_fbdemo_triforce.c
        game/m_fbdemo_wipe1.c
        game/m_field_assessment.c
        game/m_field_info.c
        game/m_field_make.c
        game/m_fishrecord.c
        game/m_flashrom.c
        game/m_font.c
        game/m_fuusen.c
        game/m_game_dlftbls.c
        game/m_gba_ovl.c
        game/m_hand_ovl.c
        game/m_handbill.c
        game/m_haniwa_ovl.c
        game/m_haniwaPortrait_ovl.c
        game/m_hboard_ovl.c
        game/m_home.c
        game/m_house.c
        game/m_huusui_room.c
        game/m_huusui_room_ovl.c
        game/m_inventory_ovl.c
        game/m_island.c
        game/m_item_name.c
        game/m_kabu_manager.c
        game/m_kankyo.c
        game/m_land.c
        game/m_ledit_ovl.c
        game/m_lib.c
        game/m_lights.c
        game/m_mail.c
        game/m_mail_check.c
        game/m_mail_check_ovl.c
        game/m_mail_password_check.c
        game/m_mailbox_ovl.c
        game/m_malloc.c
        game/m_map_ovl.c
        game/m_mark_room.c
        game/m_mark_room_ovl.c
        game/m_melody.c
        game/m_mscore_ovl.c
        game/m_msg.c
        game/m_museum.c
        game/m_museum_display.c
        game/m_mushroom.c
        game/m_music_ovl.c
        game/m_name_table.c
        game/m_needlework.c
        game/m_needlework_ovl.c
        game/m_notice.c
        game/m_notice_ovl.c
        game/m_npc.c
        game/m_npc_schedule.c
        game/m_npc_walk.c
        game/m_olib.c
        game/m_passwordChk_ovl.c
        game/m_passwordMake_ovl.c
        game/m_pause.c
        game/m_play.c
        game/m_player.c
        game/m_player_call.c
        game/m_player_lib.c
        game/m_police_box.c
        game/m_post_office.c
        game/m_prenmi.c
        game/m_private.c
        game/m_quest.c
        game/m_random_field.c
        game/m_random_field_ovl.c
        game/m_rcp.c
        game/m_repay_ovl.c
        game/m_roll_lib.c
        game/m_room_type.c
        game/m_scene.c
        game/m_scene_ftr.c
        game/m_scene_table.c
        game/m_select.c
        game/m_shop.c
        game/m_skin_matrix.c
        game/m_snowman.c
        game/m_soncho.c
        game/m_start_data_init.c
        game/m_string.c
        game/m_submenu.c
        game/m_submenu_ovl.c
        game/m_tag_ovl.c
        game/m_time.c
        game/m_timeIn_ovl.c
        game/m_titledemo.c
        game/m_trademark.c
        game/m_train_control.c
        game/m_vibctl.c
        game/m_view.c
        game/m_warning_ovl.c
        game/m_watch_my_step.c
)

# --- system ---
# System utilities: math, matrix, stacks, ucode, video manager
forest_add_library(system
    BASE_DIR        ${CMAKE_SOURCE_DIR}/src
    INTERFACE_TARGET forest_rel
    SOURCES
        system/sys_dynamic.c
        system/sys_math.c
        system/sys_math3d.c
        system/sys_math_atan.c
        system/sys_matrix.c
        system/sys_romcheck.c
        system/sys_stacks.c
        system/sys_ucode.c
        system/sys_vimgr.c
)

# --- dataobject ---
# Static data tables: NPC models/textures, field data, scene data, item data, etc.
# This library contains ~3200 data files. We use GLOB_RECURSE to avoid listing them all.
file(GLOB_RECURSE DATAOBJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/data/*.c
)
if(DATAOBJECT_SOURCES)
    add_library(dataobject STATIC ${DATAOBJECT_SOURCES})
    target_link_libraries(dataobject PUBLIC forest_rel)
endif()

# ==============================================================================
# Build Targets
# ==============================================================================

# Collect all libraries into lists for convenience
set(FOREST_STATIC_LIBS
    boot
    libforest
    libu64
    libc64
    libultra
    libjsys
    J2DGraph
    JFramework
    JGadget
    JKernel
    JSupport
    JUtility
    jaudio_NES_game
    jaudio_NES_internal
    dolphin_pc_stubs
    Famicom
)

set(FOREST_REL_LIBS
    foresta
    actor
    actor_npc
    actor_npc_event
    actor_tool
    bg_item
    effect
    game
    system
)

if(TARGET dataobject)
    list(APPEND FOREST_REL_LIBS dataobject)
endif()

# All libraries combined
set(FOREST_ALL_LIBS ${FOREST_STATIC_LIBS} ${FOREST_REL_LIBS})

# ==============================================================================
# DOL Executable — Main game binary
# ==============================================================================
# On the GameCube, the DOL is the main executable containing engine code
# (Dolphin SDK, JSystem, libforest, etc.) and the bootstrap (boot.c).
# For the PC port, this is a standard executable with a PC entry point.
# Engine symbols are exported so the REL module (DLL) can call them.

add_executable(forest_dol src/pc/dol_main.c)
target_link_libraries(forest_dol PRIVATE
    ${FOREST_STATIC_LIBS}
    aurora::core
    aurora::pad
    aurora::si
    aurora::main
    aurora::mtx
    aurora::vi
    aurora::os
)
set_target_properties(forest_dol PROPERTIES
    OUTPUT_NAME "animal_crossing"
    ENABLE_EXPORTS TRUE
)

# Export all symbols from the executable so the REL DLL can link against them.
# ENABLE_EXPORTS generates the import library. We force all symbols to be exported
# without needing __declspec(dllexport) annotations.
if(MSVC OR CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    # Keep all objects from engine static libs in the EXE so their symbols are
    # available to the REL import library.
    foreach(_lib IN LISTS FOREST_STATIC_LIBS)
        target_link_options(forest_dol PRIVATE
            "/WHOLEARCHIVE:$<TARGET_FILE:${_lib}>"
        )
    endforeach()
    # clang-cl / MSVC: CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS + ENABLE_EXPORTS handles export flags.
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(forest_dol PRIVATE -Wl,--export-all-symbols)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang often uses lld on Linux, which does not support --export-all-symbols.
    # Rely on -rdynamic / --export-dynamic (already set) for symbol export.
endif()

# ==============================================================================
# REL Shared Library — Foresta game logic module
# ==============================================================================
# On the GameCube, foresta.rel is a relocatable module loaded at runtime
# containing all game logic (actors, scenes, events, etc.).
# For the PC port, this is a shared library (DLL on Windows, .so on Linux).

add_library(foresta_rel SHARED
    src/pc/rel_main.c
    src/pc/foresta_rel_stubs.c
)
target_link_libraries(foresta_rel PRIVATE forest_common)
target_link_libraries(foresta_rel PRIVATE aurora::mtx)
set_target_properties(foresta_rel PROPERTIES
    OUTPUT_NAME "foresta_mod"
)

# Link all REL static libraries into the shared library.
# --whole-archive / /WHOLEARCHIVE ensures every object from the static libs is
# included, not just those that resolve internal references. This is necessary
# because game functions are called by the executable at runtime, not internally.
if(MSVC OR (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_SIMULATE_ID STREQUAL "MSVC"))
    target_link_libraries(foresta_rel PRIVATE ${FOREST_REL_LIBS})
    foreach(_lib IN LISTS FOREST_REL_LIBS)
        target_link_options(foresta_rel PRIVATE
            "/WHOLEARCHIVE:$<TARGET_FILE:${_lib}>"
        )
    endforeach()
else()
    target_link_libraries(foresta_rel PRIVATE
        -Wl,--whole-archive
        ${FOREST_REL_LIBS}
        -Wl,--no-whole-archive
    )
endif()

# Also link static DOL engine libs directly so REL can resolve symbols even when
# they are not exported via the executable import library on PC.
set(FOREST_STATIC_LIBS_FOR_REL ${FOREST_STATIC_LIBS})
list(REMOVE_ITEM FOREST_STATIC_LIBS_FOR_REL boot)
target_link_libraries(foresta_rel PRIVATE ${FOREST_STATIC_LIBS_FOR_REL})

# Link against the DOL executable's exported symbols.
# CMake resolves this via the import library generated by ENABLE_EXPORTS.
target_link_libraries(foresta_rel PRIVATE forest_dol)
