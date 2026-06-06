/*
 *   ScrollManager — centralized scroll/scene system state
 */

#include "scroll_manager.h"

// --- グローバルインスタンス ---
ScrollManager Scroller;

// --- 後方互換用参照ラッパー ---
ScrollState& ScrollInfo = Scroller.scroll;
SceneState& SclInfo = Scroller.scene;
int& SclKeyWaitCount = Scroller.key_wait_count;
