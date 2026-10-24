#include "car_command.h"

static int16_t speed;
static char direction;
static int16_t travel; /* +1 前进，-1 后退；speed 始终只保存速度大小。 */

void car_command_init(void)
{
    speed = CAR_SPEED_INITIAL;
    direction = 'f';
    travel = 1;
}

uint8_t car_command_handle(char command, car_target_t *target)
{
    switch (command)
    {
        case '+':
            if (speed < CAR_SPEED_MAX) speed++;
            break;
        case '-':
            if (speed > 0) speed--;
            break;
        case 'l': case 'L':
            direction = 'l';
            break;
        case 'r': case 'R':
            direction = 'r';
            break;
        case 'f': case 'F':
            travel = 1;
            direction = 'f';
            break;
        case 'b': case 'B':
            travel = -1;
            direction = 'f';
            break;
        case 's': case 'S':
            speed = 0;
            direction = 'f';
            break;
        default:
            return 0U; /* 包括串口助手自动附带的回车和换行。 */
    }

    target->left = speed * travel;
    target->right = speed * travel;

    /* l/r 按车头转动方向定义，始终停一轮、转另一轮。
     * 前进左转停左轮；后退左转要停右轮，才能让车头仍然向左转。
     */
    if ((direction == 'l' && travel > 0) || (direction == 'r' && travel < 0))
        target->left = 0;
    else if ((direction == 'r' && travel > 0) || (direction == 'l' && travel < 0))
        target->right = 0;

    return 1U;
}
