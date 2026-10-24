// GLOBAL: 0x509f00
extern "C" short g_sinTable[128];
// GLOBAL: 0x50a000
extern "C" short g_cosTable[512];
// GLOBAL: 0x509ef0
extern "C" double g_radToAngle16;
// GLOBAL: 0x509ef8
extern "C" double g_angle16ToRad;
// GLOBAL: 0x50a400
extern "C" int g_rotationMatrix[9];

// FUNCTION: 0x4b70a0
extern "C" __declspec(naked) int __cdecl FixedSine(unsigned short angle)
{
    __asm {
        push ebp
        mov ebp, esp
        push esi
        push ebx
        lea esi, g_sinTable
        movzx ebx, word ptr [ebp + 8]
        shr ebx, 6
        and ebx, 0xfffe
        movsx eax, word ptr [ebx + esi]
        pop ebx
        pop esi
        leave
        ret

        // ENTRY: 0x4b70c0
    L004b70c0:
        push ebp
        mov ebp, esp
        push esi
        push ebx
        lea esi, g_cosTable
        movzx ebx, word ptr [ebp + 8]
        shr ebx, 6
        and ebx, 0xfffe
        movsx eax, word ptr [ebx + esi]
        pop ebx
        pop esi
        leave
        ret

        // ENTRY: 0x4b70e0
        push ebp
        mov ebp, esp
        push edx
        mov eax, [ebp + 8]
        imul dword ptr [ebp + 0xc]
        mov eax, edx
        pop edx
        leave
        ret

        // ENTRY: 0x4b70ef _ScaledSin
        push ebp
        mov ebp, esp
        push esi
        push ebx
        push edx
        lea esi, g_sinTable
        movsx ebx, word ptr [ebp + 8]
        add ebx, 0x20
        shr ebx, 6
        and ebx, 0x3fe
        movsx eax, word ptr [ebx + esi]
        imul dword ptr [ebp + 0xc]
        add eax, 0x1000
        adc edx, 0
        shrd eax, edx, 13
        pop edx
        pop ebx
        pop esi
        leave
        ret

        // ENTRY: 0x4b7123 _ScaledCos
        push ebp
        mov ebp, esp
        push esi
        push ebx
        push edx
        lea esi, g_sinTable
        movzx ebx, word ptr [ebp + 8]
        add ebx, 0x4020
        shr ebx, 6
        and ebx, 0x3fe
        movsx eax, word ptr [ebx + esi]
        imul dword ptr [ebp + 0xc]
        add eax, 0x1000
        adc edx, 0
        shrd eax, edx, 13
        pop edx
        pop ebx
        pop esi
        leave
        ret

        // ENTRY: 0x4b715a _AngleOfVector
        push ebp
        mov ebp, esp
        fild dword ptr [ebp + 8]
        fild dword ptr [ebp + 0xc]
        fpatan
        fmul g_radToAngle16
        fistp dword ptr [ebp + 8]
        mov eax, [ebp + 8]
        leave
        ret

        // ENTRY: 0x4b7173 _RotatePoint2D
        push ebp
        mov ebp, esp
        cmp word ptr [ebp + 8], 0
        je L004b71a5
        mov eax, [ebp + 0xc]
        fild dword ptr [eax + 4]
        fild dword ptr [eax]
        fld st(1)
        fld st(1)
        fild word ptr [ebp + 8]
        fmul g_angle16ToRad
        fsincos
        fmul st(5), st
        fmulp st(4), st
        fmul st(2), st
        fmulp st(1), st
        faddp st(3), st
        fsubp st(1), st
        fistp dword ptr [eax]
        fistp dword ptr [eax + 4]
    L004b71a5:
        leave
        ret

        // ENTRY: 0x4b71a7
        push ebp
        mov ebp, esp
        add esp, -0x18
        push esi
        push edi
        push ebx
        push edx
        mov esi, [ebp + 8]
        push word ptr [esi + 0xe]
        call L004b70c0
        mov [ebp - 8], eax
        mov edi, eax
        push word ptr [esi + 0x10]
        call L004b70c0
        add sp, 4
        mov [ebp - 0xc], eax
        imul edi
        shrd eax, edx, 13
        mov g_rotationMatrix, eax
        push word ptr [esi + 0xc]
        call L004b70c0
        mov [ebp - 4], eax
        mov edi, eax
        push word ptr [esi + 0x10]
        call FixedSine
        add sp, 4
        mov [ebp - 0x18], eax
        imul edi
        shrd eax, edx, 13
        push eax
        push word ptr [esi + 0xc]
        call FixedSine
        mov [ebp - 0x10], eax
        mov edi, eax
        push word ptr [esi + 0xe]
        call FixedSine
        add sp, 4
        mov [ebp - 0x14], eax
        imul edi
        shrd eax, edx, 13
        imul dword ptr [ebp - 0xc]
        shrd eax, edx, 13
        pop ebx
        add eax, ebx
        mov g_rotationMatrix[4], eax
        mov eax, [ebp - 0x10]
        imul dword ptr [ebp - 0x18]
        shrd eax, edx, 13
        mov edi, eax
        mov eax, [ebp - 4]
        imul dword ptr [ebp - 0x14]
        shrd eax, edx, 13
        imul dword ptr [ebp - 0xc]
        shrd eax, edx, 13
        sub edi, eax
        mov g_rotationMatrix[8], edi
        mov eax, [ebp - 8]
        imul dword ptr [ebp - 0x18]
        shrd eax, edx, 13
        neg eax
        mov g_rotationMatrix[12], eax
        mov eax, [ebp - 4]
        imul dword ptr [ebp - 0xc]
        shrd eax, edx, 13
        mov edi, eax
        mov eax, [ebp - 0x10]
        imul dword ptr [ebp - 0x14]
        shrd eax, edx, 13
        imul dword ptr [ebp - 0x18]
        shrd eax, edx, 13
        sub edi, eax
        mov g_rotationMatrix[16], edi
        mov eax, [ebp - 0x10]
        imul dword ptr [ebp - 0xc]
        shrd eax, edx, 13
        mov edi, eax
        mov eax, [ebp - 4]
        imul dword ptr [ebp - 0x14]
        shrd eax, edx, 13
        imul dword ptr [ebp - 0x18]
        shrd eax, edx, 13
        add eax, edi
        mov g_rotationMatrix[20], eax
        mov eax, [ebp - 0x14]
        mov g_rotationMatrix[24], eax
        mov eax, [ebp - 0x10]
        imul dword ptr [ebp - 8]
        shrd eax, edx, 13
        neg eax
        mov g_rotationMatrix[28], eax
        mov eax, [ebp - 4]
        imul dword ptr [ebp - 8]
        shrd eax, edx, 13
        mov g_rotationMatrix[32], eax
        pop edx
        pop ebx
        pop edi
        pop esi
        leave
        ret

        // ENTRY: 0x4b72e2
        push ebp
        mov ebp, esp
        push esi
        push edi
        push ecx
        push edx
        mov esi, [ebp + 8]
        mov edi, [ebp + 0xc]
        mov eax, [esi]
        imul g_rotationMatrix
        shrd eax, edx, 13
        mov ecx, eax
        mov eax, [esi + 4]
        imul g_rotationMatrix[12]
        shrd eax, edx, 13
        add ecx, eax
        mov eax, [esi + 8]
        imul g_rotationMatrix[24]
        shrd eax, edx, 13
        add eax, ecx
        mov [edi], eax
        mov eax, [esi]
        imul g_rotationMatrix[4]
        shrd eax, edx, 13
        mov ecx, eax
        mov eax, [esi + 4]
        imul g_rotationMatrix[16]
        shrd eax, edx, 13
        add ecx, eax
        mov eax, [esi + 8]
        imul g_rotationMatrix[28]
        shrd eax, edx, 13
        add eax, ecx
        mov [edi + 4], eax
        mov eax, [esi]
        imul g_rotationMatrix[8]
        shrd eax, edx, 13
        mov ecx, eax
        mov eax, [esi + 4]
        imul g_rotationMatrix[20]
        shrd eax, edx, 13
        add ecx, eax
        mov eax, [esi + 8]
        imul g_rotationMatrix[32]
        shrd eax, edx, 13
        add eax, ecx
        mov [edi + 8], eax
        pop edx
        pop ecx
        pop edi
        pop esi
        leave
        ret

        // ENTRY: 0x4b7381 _MulDivTrunc
        push ebp
        mov ebp, esp
        push ebx
        push ecx
        push edx
        mov ecx, [ebp + 0x10]
        or ecx, ecx
        je L004b739d
        mov ebx, [ebp + 0xc]
        mov eax, [ebp + 8]
        imul ebx
        idiv ecx
        pop edx
        pop ecx
        pop ebx
        leave
        ret
    L004b739d:
        xor eax, eax
        pop edx
        pop ecx
        pop ebx
        leave
        ret
    }
}
