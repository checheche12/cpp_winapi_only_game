// Pixel_Bounce.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "Pixel_Bounce.h"

HBITMAP gBmp = nullptr;        // 공 이미지 (스프라이트, bitmap 형식으로 저장)
int gBW = 0, gBH = 0;
int gX = 50, gY = 50;
double gPX = 50, gPY = 50;   // 실수 좌표 -> 랜더 할때는 정수 단위로 저장한 값을 써야 함.
double gVX = 0, gVY = 0;    // 속도(px/s)
double gGravity = 1800.0;    // 중력
double gBounceHeight = 180.0;  // 어디서 떨어지더라도 항상 해당 높이까지 튐 (이거는 좀 더 조정의 여지가 있음. 이 높이를 아무리 낮은곳에서 튀어도 올라가야하는 최소치로 해서 튄다던가 등등...)
double gRestitution = 0.55;  // 반발계수(0~1, 낮을수록 덜 튐) -> 위에것과 조합해서 쓸 때 필요해서 놔 둠. 해당 비율만큼 튈 때 에너지를 잃는다.
double gMoveSpeed = 420.0;  // 좌우 이동 속도
double gJumpImpulse = 620.0;  // 점프 임펄스
int gSpeed = 6; // 프레임당 이동 픽셀

/*
 Logical 하게 윈도우를 그린 뒤, 화면 업데이트 할 때는 현재 사이즈를 받아와서, 현재사이즈 기반으로 재 계산해서 그린다.
*/
const int LOGICAL_WIDTH = 1024;
const int LOGICAL_HEIGHT = 800;

// key 에 해당하는 색을 투명하게 그린다.
void DrawBmpKeyed(HDC dest, int x, int y, HBITMAP bmp, int w, int h, COLORREF key) {
    HDC src = CreateCompatibleDC(dest);
    HGDIOBJ old = SelectObject(src, bmp);
    TransparentBlt(dest, x, y, w, h, src, 0, 0, w, h, key);
    SelectObject(src, old);
    DeleteDC(src);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 이미지 로드
        gBmp = (HBITMAP)LoadImage(nullptr, L"ball.bmp", IMAGE_BITMAP, 0, 0,
            LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (gBmp) {
            BITMAP bm{}; GetObject(gBmp, sizeof(bm), &bm);
            gBW = bm.bmWidth; gBH = bm.bmHeight;
        }
        // 60fps 근사 타이머(약 16ms)
        SetTimer(hWnd, 1, 16, nullptr);
        return 0;
    }
    case WM_TIMER: {
        // 고정 dt
        const double dt = 0.016;

        // 입력은 속도 기반으로 처리
        double ax = 0.0;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000)  ax = -gMoveSpeed;
        else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) ax = gMoveSpeed;
        else ax = 0.0;

        // 가속도를 쓰고 싶으면 gVX += ax*dt;
        gVX = ax;

        /*
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            gVY = -gJumpImpulse;
        }
        */

        // 중력 적용
        gVY += gGravity * dt;
        gPX += gVX * dt;
        gPY += gVY * dt;

        // 충돌
        RECT rc; GetClientRect(hWnd, &rc);
        // 좌우
        if (gPX < 0) {
            gPX = 0;
            gVX = -gVX;
        }
        else if (gPX + gBW > LOGICAL_WIDTH) {
            gPX = LOGICAL_WIDTH - gBW;
            gVX = -gVX;
        }
        // 상하
        if (gPY + gBH > LOGICAL_HEIGHT) {
            gPY = LOGICAL_HEIGHT - gBH;
            // 반드시 해당 높이까지 올라가도록 에너지 조절
            gVY = -std::sqrt(2.0 * gGravity * gBounceHeight);
        }
        else if (gPY + gBH > LOGICAL_HEIGHT) {
            gPY = LOGICAL_HEIGHT - gBH;
            gVY = -gVY;
            // 너무 느리다면...
            if (fabs(gVY) < 60) gVY = 0;
        }

        // 랜더링을 위해서는 정수 좌표로 변환 해야 함.
        gX = (int)gPX;
        gY = (int)gPY;

        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT wr; GetClientRect(hWnd, &wr);
        int winW = wr.right; int winH = wr.bottom;

        static HBITMAP backBmp = nullptr;
        static HDC memDC = nullptr;

        if (!memDC) {
            HDC th = GetDC(hWnd);
            memDC = CreateCompatibleDC(th);
            backBmp = CreateCompatibleBitmap(th, LOGICAL_WIDTH, LOGICAL_HEIGHT);
            SelectObject(memDC, backBmp);
            ReleaseDC(hWnd, th);
        }

        HBRUSH bg = CreateSolidBrush(RGB(24, 24, 32));
        RECT lr{ 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT };
        
        FillRect(memDC, &lr, bg); DeleteObject(bg);

        if (gBmp) {
            DrawBmpKeyed(memDC, (int)gPX, (int)gPY, gBmp, gBW, gBH, RGB(255, 0, 255));
        }

        double sx = (double)winW / LOGICAL_WIDTH;
        double sy = (double)winH / LOGICAL_HEIGHT;

        double s = (sx < sy ? sx : sy);
        int vw = (int)(LOGICAL_WIDTH * s);
        int vh = (int)(LOGICAL_HEIGHT * s);
        int vx = (winW - vw) / 2;
        int vy = (winH - vh) / 2;

        StretchBlt(hdc, vx, vy, vw, vh, memDC, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        if (gBmp) { DeleteObject(gBmp); gBmp = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
	const wchar_t* CLASS_NAME = L"GameWindowClass";

	WNDCLASS wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(
		0, CLASS_NAME, L"WinAPI Game", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		nullptr, nullptr, hInst, nullptr);

	ShowWindow(hWnd, nCmdShow);

	// 메시지 루프(기본형)
	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
