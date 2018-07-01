#include <QList>
#include "ScanWorkerState.h"

QString ScanWorkerState::ToString(enum_type type)
{
    static QList<QString> names = { "Available", "Copying", "Ready", "Scanning", "ReadingResults", "Complete", "Error", "Done" };
    return names.at(type);
}
