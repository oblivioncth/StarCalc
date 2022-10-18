// Unit Include
#include "ballotbox_p.h"

// Qx Includes
#include <qx/core/qx-dsvtable.h>
#include <qx/io/qx-common-io.h>

// Project Includes
#include "categoryconfig_p.h"

namespace Star
{

//===============================================================================================================
// RefBallotBox
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
RefBallotBox::RefBallotBox() {}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
const QList<RefCategory>& RefBallotBox::categories() const { return mCategories; }
const QList<RefBallot>& RefBallotBox::ballots() const { return mBallots; }

//===============================================================================================================
// RefBallotBox::Reader
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Protected:
RefBallotBox::Reader::Reader(RefBallotBox* targetBox, const QString& filePath, const RefCategoryConfig* categoryConfig) :
    mTargetBox(targetBox),
    mCsvFile(filePath),
    mCategoryConfig(categoryConfig),
    mExpectedFieldCount(STATIC_FIELD_COUNT + mCategoryConfig->totalNominees())
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError RefBallotBox::Reader::parseCategories(const QList<QVariant>& headingsRow)
{

    // Fill out categories
    qsizetype cIdx = STATIC_FIELD_COUNT; // Skip known headings

    for(const RefCategoryHeader& ch : mCategoryConfig->headers())
    {
        // Read nominees directly into Category
        RefCategory category{.name = ch.name, .nominees = {}};
        QStringList& nominees = category.nominees;

        for(uint i = 0; i < ch.nomineeCount; i++, cIdx++)
        {
            QString nomineeField = headingsRow[cIdx].toString();
            if(nomineeField.isEmpty())
                return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_BLANK_VALUE);

            nominees += nomineeField;
        }

        // Add category to box
        mTargetBox->mCategories.append(category);
    }

    return Qx::GenericError();
}

Qx::GenericError RefBallotBox::Reader::parseBallot(const QList<QVariant>& ballotRow)
{
    // Ignore lines with all empty fields
    bool allEmpty = true;
    for(const QVariant& field : ballotRow)
    {
        if(!field.toString().isEmpty())
        {
            allEmpty = false;
            break;
        }
    }

    if(allEmpty)
        return Qx::GenericError();

    // Read submission date
    QDate submitted = QDate::fromString(ballotRow[SUBMISSION_DATE_INDEX].toString(), "d-MMM-yy").addYears(100);
    if(!submitted.isValid())
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_INVALID_DATE);

    // Read voter name
    QString voterName = ballotRow[MEMBER_NAME_INDEX].toString();
    if(voterName.isEmpty())
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_BLANK_VALUE);

    // Create ballot with existing info
    RefBallot ballot{.voter = voterName, .submissionDate = submitted, .votes = {}};

    // Read votes by category
    qsizetype cIdx = STATIC_FIELD_COUNT; // Skip known headings

    for(const RefCategoryHeader& ch : mCategoryConfig->headers())
    {
        QList<uint> categoryVotes;

        for(uint i = 0; i < ch.nomineeCount; i++, cIdx++)
        {
            QString voteField = ballotRow[cIdx].toString();

            // If field is blank, treat it as a 0
            if(voteField.isEmpty())
            {
                categoryVotes.append(0);
                break;
            }

            // Get value from field
            bool validValue;
            uint vote = voteField.toUInt(&validValue);

            if(!validValue || vote > 5)
                return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_INVALID_VOTE);

            categoryVotes.append(vote);
        }

        ballot.votes.append(categoryVotes);
    }

    // Add ballot to box
    mTargetBox->mBallots.append(ballot);

    return Qx::GenericError();
}

//Public:
Qx::GenericError RefBallotBox::Reader::readInto()
{
    /* TODO: May want to add a check that makes sure they're aren't duplicate category names, or other redundancies that would break things.
     * Unlikely to occur though as this would be an issue with input data collection
     */

    // Error tracking
    Qx::GenericError errorStatus;

    // Quickly check if file is empty
    if(Qx::fileIsEmpty(mCsvFile))
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_EMPTY);

    // Read whole CSV into memory as raw data
    QByteArray csv;
    Qx::IoOpReport readReport = Qx::readBytesFromFile(csv, mCsvFile);
    if(readReport.isFailure())
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(readReport.outcomeInfo());

    // Parse into DsvTable
    Qx::DsvParseError dsvError;
    Qx::DsvTable csvTable = Qx::DsvTable::fromDsv(csv, ',', '"', &dsvError);
    if(dsvError.error() != Qx::DsvParseError::NoError)
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(dsvError.errorString());

    // Ensure the minimum amount of rows are present
    if(csvTable.rowCount() < 3)
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_INVALID_ROW_COUNT);

    // Ensure the column count is correct
    if(csvTable.columnCount() != mExpectedFieldCount)
        return Qx::GenericError(ERROR_TEMPLATE).setSecondaryInfo(ERR_INVALID_COLUMN_COUNT);

    // Separate headings from ballot rows
    QList<QVariant> headingRow = csvTable.takeFirstRow();

    // Process headings
    if((errorStatus = parseCategories(headingRow)).isValid())
        return errorStatus;

    // Process ballots
    for(auto itr = csvTable.rowBegin(); itr != csvTable.rowEnd(); itr++)
    {
        if((errorStatus = parseBallot(*itr)).isValid())
            return errorStatus;
    }

    return errorStatus;
}

}
