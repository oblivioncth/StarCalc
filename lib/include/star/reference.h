#ifndef REFERENCE_H
#define REFERENCE_H

// Project Includes
#include "star/election.h"
#include "star/expectedelectionresult.h"
#include "star/calculator.h"

namespace Star
{

enum class ReferenceErrorType { NoError, CategoryConfig, BallotBox, ExpectedResult, CalcOptions };

struct ReferenceError
{
    ReferenceErrorType type = ReferenceErrorType::NoError;
    QString error;
    QString errorDetails;

    bool isValid() { return type != ReferenceErrorType::NoError; }
};

//-Namespace-Functions--------------------------------------------------------------------------------
ReferenceError electionsFromReferenceInput(QList<Election>& returnBuffer,
                                           const QString& categoryConfigPath,
                                           const QString& ballotBoxPath);

ReferenceError expectedResultsFromReferenceInput(QList<ExpectedElectionResult>& returnBuffer,
                                                 const QString& resultSetPath);

ReferenceError calculatorOptionsFromReferenceInput(Star::Calculator::Options& returnBuffer,
                                                   const QString& calcOptionsPath);
}

#endif // REFERENCE_H
