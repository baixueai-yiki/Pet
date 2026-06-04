#pragma once

#include "../PetActor.h"

// 启动时初始化：注册输入回调、订阅定时事件、设置睡觉时间
class RunComponent : public PetComponent
{
public:
    void OnInit(PetActor& actor) override;
    void OnShutdown(PetActor& actor) override;
};
