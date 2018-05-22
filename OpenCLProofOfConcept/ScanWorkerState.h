#pragma once

struct ScanWorkerState
{
    enum enum_type {
        Available,
        Copying,
        Ready,
        Scanning,
        Complete,
        Error
    };
};
