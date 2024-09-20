

#include "LvglApp.h"

#include "lvgl/demos/lv_demos.h"

extern BOOLEAN  mEscExit;

BOOLEAN  mTickSupport = FALSE;

void efi_disp_flush(lv_display_t * disp, const lv_area_t * area, lv_color32_t * color32_p)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_STATUS                         Status;
  UINTN                              Width, Heigth;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status)) {
    return;
  }

  Width = area->x2 - area->x1 + 1;
  Heigth = area->y2 - area->y1 + 1;

  Status =  GraphicsOutput->Blt (
                              GraphicsOutput,
                              (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)color32_p,
                              EfiBltBufferToVideo,
                              0,
                              0,
                              (UINTN)area->x1,
                              (UINTN)area->y1,
                              Width,
                              Heigth,
                              0
                              );


  lv_display_flush_ready(disp);
}


#if LV_USE_LOG
static void efi_lv_log_print(lv_log_level_t level, const char * buf)
{
    static const int priority[LV_LOG_LEVEL_NUM] = {
        DEBUG_VERBOSE, DEBUG_INFO, DEBUG_WARN, DEBUG_ERROR, DEBUG_INFO, DEBUG_INIT
    };

    DebugPrint (priority[level], "[LVGL] %a\n", buf);
}
#endif


static uint32_t tick_get_cb(void)
{
  return (UINT32) DivU64x32 (GetTimeInNanoSecond (GetPerformanceCounter()), 1000 * 1000);
}

VOID
EFIAPI
UefiLvglTickInit (
  VOID
  )
{
  if (GetPerformanceCounter()) {
    mTickSupport = TRUE;
    lv_tick_set_cb(tick_get_cb);
  }
}

VOID
EFIAPI
TestDemo (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_STATUS                         Status;
  UINTN                              Width, Heigth;
  UINTN                              BufSize;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status)) {
    return;
  }

  lv_init();

  UefiLvglTickInit();

#if LV_USE_LOG
  lv_log_register_print_cb (efi_lv_log_print);
#endif

  Width  = GraphicsOutput->Mode->Info->HorizontalResolution;
  Heigth = GraphicsOutput->Mode->Info->VerticalResolution;
  lv_display_t *display = lv_display_create(Width, Heigth);

  static lv_color32_t *buf1;
  static lv_color32_t *buf2;
  BufSize = Width * Heigth * sizeof (lv_color32_t);
  buf1 = malloc (BufSize);
  buf2 = malloc (BufSize);
  if (!buf1 || !buf2) {
    DebugPrint (DEBUG_ERROR, "Cannot init lv_display, deinit and return.\n");
    lv_deinit();
    return;
  }

  lv_display_set_buffers(display, buf1, buf2, BufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, (lv_display_flush_cb_t)efi_disp_flush);

  lv_port_indev_init(display);

  // lv_demo_keypad_encoder();
  lv_demo_widgets();
  // lv_demo_benchmark();

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  while (1) {
    if (mEscExit) {
      break;
    }

    lv_timer_handler();

    gBS->Stall (10 * 1000);
    if (!mTickSupport) {
      lv_tick_inc(10);
    }
  }

  lv_deinit();

  lv_port_indev_close();

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  return;
}


/**
  Entry point for LvglApp.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval Status         Whether this function complete successfully.

**/
EFI_STATUS
EFIAPI
LvglAppEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  TestDemo();

  return Status;
}
