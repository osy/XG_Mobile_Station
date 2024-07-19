/**
 * This is the reverse engineered UEFI BIOS functions for reconnecting XGM on boot.
 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0001172c(void)

{
  byte bVar1;
  undefined local_res8 [8];
  undefined local_res10 [8];
  char local_res18 [4];
  longlong local_res20;
  
  local_res20 = 0;
  (**(code **)(DAT_0001fde8 + 0x140))(&DAT_0001d420,0,&local_res20);
  (**(code **)(DAT_0001fde8 + 0x140))(&DAT_0001d420,0,&local_res20);
                    /* read from EC */
  (**(code **)(local_res20 + 200))(0x1c,local_res18,4,1);
  out(0x72,0x7f);
  bVar1 = in(0x73);
  if (bVar1 == 0) {
    bVar1 = 0;
    out(0x72,0x7f);
    out(0x73,1);
  }
                    /* UMAF == 0x02 */
  if ((bVar1 & 4) == 0) {
                    /* GPUM != 0 */
                    /* EGIF [0x00] == 0x40 (not established) */
                    /* EGIF [0x01] != 3 (connected & locked) */
                    /* EGIF [0x02] != 0 (power loss) */
    if ((((bVar1 & 2) != 0) && (local_res18[0] == 0x40)) &&
       (((local_res18[1] & 3U) != 3 ||
        (((local_res18[2] & 1U) != 0 || ((local_res18[3] & 2U) == 0)))))) {
                    /* GPUV = 1 (iGPU) */
      bVar1 = bVar1 & 0xfc | 0x40;
      out(0x72,0x7f);
      out(0x73,bVar1);
                    /* turn off eGPU */
      local_res18[0] = 0xa;
                    /* write to EC */
      (**(code **)(local_res20 + 200))(0x1c,local_res18,1,0);
    }
  }
  else {
    if (local_res18[0] == 0x40) {
      _DAT_fed815a8 = _DAT_fed815a8 & 0xffbfffff;
      FUN_00013d30(50000);
      _DAT_fed81708 = _DAT_fed81708 | 0x400000;
    }
    else {
      _DAT_fed815a0 = _DAT_fed815a0 & 0xffbfffff;
      FUN_00013d30(50000);
      _DAT_fed8176c = _DAT_fed8176c | 0x400000;
    }
    FUN_00013d30(50000);
  }
                    /* GPUV != 0 */
  if ((bVar1 & 0x40) != 0) {
    reconnect_xgm();
  }
  local_res8[0] = 0;
  local_res10[0] = 0;
                    /* UMAF != 0 */
  if ((_DAT_e0100000 == -1) || ((bVar1 & 0xc) != 0)) {
    (**(code **)(local_res20 + 200))(0xff08,local_res10,1,0);
    local_res8[0] = 1;
  }
  else {
    (**(code **)(local_res20 + 200))(0xff07,local_res10,1,0);
    local_res8[0] = 0;
  }
  (**(code **)(local_res20 + 200))(0x45,local_res8,1,0);
  return;
}


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void reconnect_xgm(void)

{
  uint uVar1;
  byte bVar2;
  longlong lVar3;
  ulonglong uVar4;
  byte bVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  byte bVar9;
  uint uVar10;
  uint uVar11;
  byte local_res8;
  byte local_res10;
  char local_res18 [4];
  short local_res20 [2];
  longlong ecHandle;
  uint imgFileGuid [4];
  ulonglong imgHandle2;
  longlong *imgHandle1;
  longlong local_80;
  ulonglong local_78;
  longlong local_70;
  longlong local_68;
  ulonglong *local_60;
  ulonglong local_58;
  
  uVar7 = 0;
  imgFileGuid[0] = 0x4bd47fc9;
  imgHandle1 = (longlong *)0x0;
  bVar9 = 0;
  imgHandle2 = 0;
  ecHandle = 0;
  local_res10 = 0;
  uVar10 = 0;
  imgFileGuid[1] = 0x400a46bc;
  imgFileGuid[2] = 0xb0750f94;
  imgFileGuid[3] = 0xdaffe15;
  out(0x72,0x7f);
  bVar2 = in(0x73);
  (**(code **)(DAT_0001fde8 + 0x140))(&DAT_0001d420,0,&ecHandle);
  lVar3 = (**(code **)(DAT_0001fb28 + 0x138))
                    (2,&EfiFirmwareVolume2ProtocolGuid,0,&local_78,&local_80);
  if (-1 < lVar3) {
                    /* get XGM disconnect splash image */
    lVar3 = FUN_0000e4ac(imgFileGuid,&imgHandle1,&imgHandle2,&local_78,&local_80);
    if (-1 < lVar3) {
      (**(code **)(ecHandle + 200))(0x43,&local_res8,1);
      local_res8 = local_res8 | 0x40;
      (**(code **)(ecHandle + 200))(0x42,&local_res8);
      FUN_00001264();
      uVar6 = uVar7;
      uVar8 = uVar7;
      uVar11 = uVar7;
      if (*(int *)DAT_0001fc00[3] != 0) {
        do {
          lVar3 = (**DAT_0001fc00)(DAT_0001fc00,uVar6);
          if ((lVar3 == 0) && (local_70 != 0)) {
            uVar1 = *(uint *)(local_68 + 4);
            if ((uVar1 == 0x500) && (*(int *)(local_68 + 8) == 0x2d0)) {
              uVar10 = uVar6 & 0xff;
            }
            if (((uVar11 < uVar1) ||
                (uVar8 <= *(uint *)(local_68 + 8) && *(uint *)(local_68 + 8) != uVar8)) &&
               (*(uint *)(local_68 + 8) * 0xe <= uVar1 * 10)) {
              local_res10 = (byte)uVar6;
              uVar11 = uVar1;
              uVar8 = *(uint *)(local_68 + 8);
            }
          }
          uVar6 = uVar6 + 1;
        } while (uVar6 < *(uint *)DAT_0001fc00[3]);
      }
      if (*(uint *)(DAT_0001fc00[3] + 4) != uVar10) {
        (*DAT_0001fc00[1])(DAT_0001fc00);
      }
                    /* show splash image? */
      uVar4 = FUN_0000ba14(imgHandle1,imgHandle2,0,0,0,'\0',&local_58,&local_60);
      if (-1 < (longlong)uVar4) {
LAB_00011502:
        while( true ) {
          do {
                    /* get keyboard key */
            lVar3 = (**(code **)(*(longlong *)(DAT_0001fb20 + 0x30) + 8))
                              (*(longlong *)(DAT_0001fb20 + 0x30),local_res20);
          } while (lVar3 != 0);
                    /* retry */
          if (((local_res20[1] == L'y') || (local_res20[1] == L'Y')) || (local_res20[1] == L'\r'))
          break;
                    /* restart */
          if (((local_res20[1] - L'N'U & 0xffdf) == 0) || (local_res20[0] == 0x17)) {
                    /* GPUM = 1 (off) */
            out(0x72,0x7f);
            out(0x73,bVar2 & 0xc | 0x81);
LAB_000116c1:
            FUN_00015c88();
            FUN_00001264();
            if (*(uint *)(DAT_0001fc00[3] + 4) != (uint)local_res10) {
              (*DAT_0001fc00[1])(DAT_0001fc00);
            }
            (**(code **)(ecHandle + 200))(0x43,&local_res8,1);
            local_res8 = local_res8 & 0xbf;
            (**(code **)(ecHandle + 200))(0x42,&local_res8,1,0);
            return;
          }
        }
                    /* EC read */
        (**(code **)(ecHandle + 200))(0x1c,local_res18,4,1);
                    /* connected & locked */
                    /* AC power loss */
                    /* ??? */
        if (((local_res18[1] != '\x03') || (local_res18[2] == '\x01')) || (local_res18[3] != '\x02')
           ) {
                    /* get keyboard key */
          (**(code **)(*(longlong *)(DAT_0001fb20 + 0x30) + 8))
                    (*(longlong *)(DAT_0001fb20 + 0x30),local_res20);
          goto LAB_00011502;
        }
                    /* GPUM = 2 (on) */
        bVar5 = bVar2 & 0xbe | 2;
                    /* UMAF != 0 */
        if ((bVar2 & 4) != 0) {
                    /* UMAF = 2 */
          bVar5 = bVar2 & 0xba | 0xa;
        }
        out(0x72,0x7f);
        out(0x73,bVar5);
                    /* connect eGPU sequence */
        _DAT_fed815a0 = _DAT_fed815a0 & 0xffbfffff;
        FUN_00013d30(50000);
        _DAT_fed8176c = _DAT_fed8176c | 0x400000;
        FUN_00013d30(50000);
        local_res18[0] = 0xb;
                    /* EC write */
        (**(code **)(ecHandle + 200))(0x1c,local_res18,1,0);
                    /* EC read */
        (**(code **)(ecHandle + 200))(0x1c,local_res18,4,1);
        for (; (local_res18[0] == -0x80 && (bVar9 < 200)); bVar9 = bVar9 + 1) {
                    /* EC read */
          (**(code **)(ecHandle + 200))(0x1c,local_res18,4,1);
        }
        FUN_00013d30(10000);
        _DAT_fed81708 = _DAT_fed81708 & 0xffbfffff;
        FUN_00013d30(50000);
        do {
          uVar10 = _DAT_fed8166c;
          if (0x13 < (byte)uVar7) break;
          FUN_00013d30(50000);
          uVar7 = (uint)(byte)((byte)uVar7 + 1);
        } while ((uVar10 >> 0x10 & 1) == 0);
        FUN_00013d30(50000);
        _DAT_fed815a8 = _DAT_fed815a8 | 0x400000;
        goto LAB_000116c1;
      }
    }
  }
  return;
}

