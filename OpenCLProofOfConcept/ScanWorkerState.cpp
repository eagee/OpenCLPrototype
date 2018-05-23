#include <QList>
#include "ScanWorkerState.h"

QString ScanWorkerState::ToString(enum_type type)
{
    static QList<QString> names = { "Available", "Copying", "Ready", "Scanning", "Complete", "Error" };
    return names.at(type);
}
