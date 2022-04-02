#pragma once

//UPDATED 30/03/2022

#define OFFSET_ENTITYLIST 0x19ed718   //cl_entitylist
#define OFFSET_LOCAL_ENT 0x1d9e418    //LocalPlayer
#define OFFSET_ORIGIN 0x014c          //m_vecAbsOrigin
#define OFFSET_BONES 0x0F38            //m_bConstrainBetweenEndpoints
#define OFFSET_NAME	0x589                 //m_iName

#define OFFSET_MATRIX 0x11a210
#define OFFSET_RENDER 0x74bad90

#define OFFSET_TEAM	0x448                 //m_iTeamNum
#define OFFSET_BLEED_OUT_STATE 0x2728     //m_bleedoutState
#define OFFSET_VISIBLE_TIME 0x1b14        //CPlayer!lastVisibleTime
#define OFFSET_ITEM_ID	0x16b8
#define GLOW_TYPE 0x2C4
#define OFFSET_GLOW_ENABLE          0x3c8 //7 = enabled, 2 = disabled
#define OFFSET_GLOW_THROUGH_WALLS   0x3d0 //2 = enabled, 5 = disabled
