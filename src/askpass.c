#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ALOT 65536

void PrintHelp() {
    fprintf(stderr, "Usage:\n> askpass.exe [-p] [-m MSG]\n\n");
    fprintf(stderr, "askpass.exe will read the prompt from its standard input,\n");
    fprintf(stderr, "print it into its console, read the reply from the console\n");
    fprintf(stderr, "and write the reply to the standard output.\n");
    fprintf(stderr, "The expected use case is askpass.exe being called from another\n");
    fprintf(stderr, "process, whose standard streams were redirected, but which\n");
    fprintf(stderr, "still needs to communicate with the real user.\n\n");
    fprintf(stderr, "  -p        do not echo entered characters to the console\n");
    fprintf(stderr, "  -m MSG    use MSG as a prompt instead of reading it from STDIN\n");
}

void ProcessCommandLineArguments(int argc, char *argv[], char* prompt, BOOL *hideInput, BOOL *stdinPrompt) {
    int i;
    *hideInput = 0;
    *stdinPrompt = 1;
    for (i=1; i<argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            PrintHelp();
            exit(1);
        } else if ((strcmp(argv[i], "-m") == 0)) {
            if (i == argc - 1) {
                fprintf(stderr, "-m expects an argument\n");
                PrintHelp();
                exit(1);
            }
            i++;
            if (strlen(argv[i]) > ALOT) {
                fprintf(stderr, "maximum size of a prompt is %d", ALOT);
                exit(1);
            }
            strcpy(prompt, argv[i]);
            *stdinPrompt = 0;
        } else if (strcmp(argv[i], "-p") == 0) {
            *hideInput = 1;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            PrintHelp();
            exit(1);
        }
    }

}

int Win32Error(char* context) {
    DWORD dw = GetLastError();
    TCHAR msg[4096];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msg,
        sizeof(msg),
        NULL);
    fprintf(stderr, "plink: %s\n%s\n", context, msg);
    exit(1);
}

void GetHandles(HANDLE *hInConsole, HANDLE *hOutConsole, HANDLE *hInStandard, HANDLE *hOutStandard) {
    /* Getting handles for STDIN, STDOUT, CONIN$ and CONOUT$ */
    *hOutConsole = CreateFile("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (*hOutConsole == INVALID_HANDLE_VALUE) {
        Win32Error("Could not create a CONOUT$ handle");
    }
    *hInConsole = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, NULL);
    if (*hInConsole == INVALID_HANDLE_VALUE) {
        Win32Error("Could not create a CONIN$ handle");
    }
    *hOutStandard = GetStdHandle(STD_OUTPUT_HANDLE);
    if (*hOutStandard == INVALID_HANDLE_VALUE) {
        Win32Error("Could not create a stdout handle");
    }
    *hInStandard = GetStdHandle(STD_INPUT_HANDLE);
    if (*hInStandard == INVALID_HANDLE_VALUE) {
        Win32Error("Could not create a stdin handle");
    }
}

int main(int argc, char *argv[]) {
    int i;
    HANDLE hInConsole, hOutConsole, hInStandard, hOutStandard;
    DWORD dwWritten, dwRead, dwPromptLen, dwLastError, oldConsoleMode, newConsoleMode;
    BOOL result, hideInput, stdinPrompt;
    char pass[ALOT], prompt[ALOT];
    ProcessCommandLineArguments(argc, argv, prompt, &hideInput, &stdinPrompt);

    GetHandles(&hInConsole, &hOutConsole, &hInStandard, &hOutStandard);
    if (stdinPrompt) {
        ReadFile(hInStandard, prompt, ALOT, &dwRead, NULL) || Win32Error("Could not read from stdin");
    }
    WriteFile(hOutConsole, prompt, strlen(prompt), &dwWritten, NULL) || Win32Error("Could not write to console");
    GetConsoleMode(hInConsole, &oldConsoleMode) || Win32Error("Could not get console mode");
    newConsoleMode = oldConsoleMode | ENABLE_LINE_INPUT;
    if (hideInput) {
        newConsoleMode &= ~ENABLE_ECHO_INPUT;
    } else {
        newConsoleMode |= ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(hInConsole, newConsoleMode) || Win32Error("Could not set console mode");
    dwRead = 0;
    ReadFile(hInConsole, pass, ALOT, &dwRead, NULL) || Win32Error("Could not read from console");
    SetConsoleMode(hInConsole, oldConsoleMode) || Win32Error("Could not restore console mode");
    if (dwRead < 2) {
        /* This should not ever occur, as the \r\n should always be present
        in the end of the read string */
        Win32Error("Something went wrong and impossibly short prompt answer is read.");
    }
    WriteFile(hOutStandard, pass, dwRead - 2, &dwWritten, NULL) || Win32Error("Could not write to stdout");
    CloseHandle(hInStandard);
    CloseHandle(hOutStandard);
    CloseHandle(hInConsole);
    CloseHandle(hOutConsole);
    return 0;
}