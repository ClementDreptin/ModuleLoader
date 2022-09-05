#pragma once

// Write a success message to stdout.
void LogSuccess(const char *szMessage, ...);

// Write an error message to stderr.
void LogError(const char *szMessage, ...);

// Write an info message to stdout.
void LogInfo(const char *szMessage, ...);
