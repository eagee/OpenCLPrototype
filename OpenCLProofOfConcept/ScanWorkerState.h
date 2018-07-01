#pragma once

struct ScanWorkerState
{
    enum enum_type {
        Available,
        Copying,
        Ready,
        Scanning,
        ReadingResults,
        Complete,
        Error,
        Done
    };

    static QString ToString(ScanWorkerState::enum_type type);
};
