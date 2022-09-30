#pragma once

// Write a success message to stdout.
void LogSuccess(const char *message, ...);

// Write an error message to stderr.
void LogError(const char *message, ...);

// Write an info message to stdout.
void LogInfo(const char *message, ...);
