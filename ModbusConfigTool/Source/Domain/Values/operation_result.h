#ifndef OPERATION_RESULT_H
#define OPERATION_RESULT_H

#include <QString>

struct OperationResult
{
    bool success = true;
    QString code;
    QString field;
    QString message;
    QString detail;

    static OperationResult ok()
    {
        return OperationResult();
    }

    static OperationResult fail(const QString &codeValue,
                                const QString &fieldValue,
                                const QString &messageValue,
                                const QString &detailValue = QString())
    {
        OperationResult result;
        result.success = false;
        result.code = codeValue;
        result.field = fieldValue;
        result.message = messageValue;
        result.detail = detailValue;
        return result;
    }
};

#endif
