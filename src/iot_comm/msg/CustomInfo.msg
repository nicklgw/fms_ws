int32   task_magic      # 当次随机魔数

float64 task_time       # 当次作业时间 h
float64 task_area       # 当次作业面积 ㎡

uint8 manual_mode       # 手动控制模式 0:普通模式；1:作业模式
uint8 ray_mode          # 激光调平模式 0:手动；1:自动
uint8 angle_mode        # 倾角调平模式 0:手动；1:自动
uint8 work_state        # 上装状态 0:未工作；1:工作
uint8 vibrate_state     # 振捣状态 0:关闭；1:开启
