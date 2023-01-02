// Unit Include
#include "star/reference.h"
#include "reference/calculatoroptions_p.h"
#include "reference/categoryconfig_p.h"
#include "reference/ballotbox_p.h"
#include "reference/resultset_p.h"

/*!
 *  @file reference.h
 *
 *  @brief The reference header file provides facilities to load election data in the reference formats.
 *
 *  @par Reference Elections
 *  @parblock
 *  A list of prepared elections can be generated by providing paths to election data in the input reference format to
 *  electionsFromReferenceInput().
 *
 *  The reference input format consists of a CSV that contains the ballots of one or more categories (elections) and
 *  an INI file that describes how many categories are present,which votes belong to which category, and how many
 *  seats need to be filled.
 *
 *  The CSV should consist of a header row, followed by one row per ballot.  The first two fields of the header row
 *  don't matter, but the following fields should consist of the candidates for each category. The first field of a
 *  ballot row does not matter, while the second should contain the candidates name (for now it is unconditionally
 *  obfuscated), and finally the remainder should contain a score (0-5) that corresponds to the candidate in the
 *  above header row.
 *
 *  Example:
 *  @snippet reference.cpp Input Format CSV
 *
 *  The INI file should start with the section "Categories" and then list each category name as a key in the same
 *  order they are listed in the CSV. The value of each of those keys should be the number of candidates in those
 *  categories. Additionally, a value for the singular key "Seats" must be specified under the section "General"
 *  in order to specify how many candidates are to be elected (Bloc voting). Simply use "1" for a typical election
 *  with a single winner.
 *
 *  Example:
 *  @snippet reference.cpp Input Format INI
 *  @endparblock
 *
 *  @par Reference Expected Results
 *  @parblock
 *  A list of expected election results can be generated by providing the path to results data in the expected results
 *  reference format to expectedResultsFromReferenceInput().
 *
 *  The reference expected results format consists of a single JSON file that encapsulates an array of arrays containing
 *  filled seat data. Essentially, the data is the JSON format equivalent of QList<QList<Seat>>. The inner arrays detail
 *  seat information for each category (election), while the outer array covers all categories. For example, a data set
 *  concerning 5 categories, each with 3 seats to fill, will contain an outer JSON array with 5 elements that are each
 *  themselves JSON arrays with 3 elements. The elements of the inner arrays are JSON objects that are similar to a
 *  Seat object.
 *
 *  Example (2 categories, 2 seats each):
 *  @snippet reference.cpp Expected Results Format
 *
 *  The `firstAdv` and `secondAdv` portions of the JSON object are equivalent to the constructor
 *  Qualifier(const QSet<QString>&, const QSet<QString>&).
 *  @endparblock
 *
 *  @par Reference Calculator Options
 *  @parblock
 *  A set of Calculator::Options can be generated by providing the path to a list of options in the calculator options
 *  reference format to calculatorOptionsFromReferenceInput().
 *
 *  The reference calculator options format is simply a text file (generally with the extension 'opt) that contains
 *  one enum value from Calculator::Option as a string per line. Blank lines are ignored and an entirely empty file will
 *  result in an options set with only the flag Calculator::Option::NoOptions set.
 *
 *  Example:
 *  @snippet reference.cpp Calculator Options Format
 *  @endparblock
 *
 *  @sa ExpectedElectionResult.
 */

namespace Star
{
//-Namespace Enums-----------------------------------------------------------------------------------------------------
/*!
 *  @enum ReferenceErrorType
 *
 *  This enum describes the type of error that occurred while parsing reference format data.
 */

/*!
 *  @var ReferenceErrorType ReferenceErrorType::NoError
 *  No error occurred.
 */

/*!
 *  @var ReferenceErrorType ReferenceErrorType::CategoryConfig
 *  An error occurred while parsing a reference category config.
 */

/*!
 *  @var ReferenceErrorType ReferenceErrorType::BallotBox
 *  An error occurred while parsing a reference ballot box.
 */

/*!
 *  @var ReferenceErrorType ReferenceErrorType::ExpectedResult
 *  An error occurred while parsing a reference expected result set.
 */

//-Namespace Structs-----------------------------------------------------------------------------------------------------
/*!
 *  @struct ReferenceError star/reference.h
 *
 *  @brief The ReferenceError struct is used to report errors while parsing election/results
 *  data in their reference formats.
 */

/*!
 *  @var ReferenceErrorType ReferenceError::type
 *
 *  The type of error that occurred.
 */

/*!
 *  @var ReferenceErrorType ReferenceError::error
 *
 *  A string that holds the primary error.
 */

/*!
 *  @var ReferenceErrorType ReferenceError::errorDetails
 *
 *  A string that details regarding the error.
 */

/*!
 *  @fn bool ReferenceError::isValid()
 *
 *  Returns @c true if the reference error actually describes an error; otherwise, returns @c false.
 */

namespace
{
    QList<Election> electionTransform(const RefBallotBox& box, uint seatCount)
    {
        QList<Election> views;

        // Create an election for each category
        for(qsizetype cIdx = 0; cIdx < box.categories().size(); cIdx++)
        {
            // Get current category
            const RefCategory& rCategory = box.categories()[cIdx];

            // Initialize election builder
            Election::Builder eBuilder(rCategory.name);

            // Create category specific ballots
            for(qsizetype bIdx = 0; bIdx < box.ballots().size(); bIdx++)
            {
                // Get current ballot
                const RefBallot& rBallot = box.ballots()[bIdx];

                // Get votes that pertain to category
                const QList<int>& categoryVotes = rBallot.votes[cIdx];

                // List to fill with candidate mapped votes in standard form
                QList<Election::Vote> mappedVotes;

                // Map votes to candidates, convert to standard Vote, add to list
                for(qsizetype nIdx = 0; nIdx < categoryVotes.size(); nIdx++)
                {
                    Election::Vote stdVote{.candidate = rCategory.candidates[nIdx], .score = categoryVotes[nIdx]};
                    mappedVotes.append(stdVote);
                }

                // Create standard voter (For now, just set the anonymous name to "Voter N")
                static const QString anonTemplate = QStringLiteral("Voter %1");
                Election::Voter stdVoter{.name = rBallot.voter, .anonymousName = anonTemplate.arg(bIdx)};

                // Add ballot to builder
                eBuilder.wBallot(stdVoter, mappedVotes);
            }

            // Set seat count
            eBuilder.wSeatCount(seatCount);

            // Build election and add to list
            views.append(eBuilder.build());
        }

        return views;
    }

    ReferenceError qxGenErrToRefError(ReferenceErrorType type, const Qx::GenericError& error)
    {
        if(!error.isValid())
            return ReferenceError();

        return ReferenceError{ .type = type, .error = error.primaryInfo(), .errorDetails = error.secondaryInfo() };
    }
}

//-Namespace-Functions--------------------------------------------------------------------------------
/*!
 *  @param[out] returnBuffer A list of elections, prepared with the provided data.
 *  @param[in] categoryConfigPath The path to the category config INI file.
 *  @param[in] ballotBoxPath The path to the ballot box CSV file.
 *  @return An error object containing error details if the operation fails.
 */
ReferenceError electionsFromReferenceInput(QList<Election>& returnBuffer,
                                            const QString& categoryConfigPath,
                                            const QString& ballotBoxPath)
{
    // Clear return buffer
    returnBuffer.clear();

    // Status tracker
    Qx::GenericError errorStatus;

    // Read category config
    RefCategoryConfig cc;
    RefCategoryConfig::Reader ccReader(&cc, categoryConfigPath);
    if((errorStatus = ccReader.readInto()).isValid())
        return qxGenErrToRefError(ReferenceErrorType::CategoryConfig, errorStatus);

    // Read ballot box
    RefBallotBox bb;
    RefBallotBox::Reader bbReader(&bb, ballotBoxPath, &cc);
    if((errorStatus = bbReader.readInto()).isValid())
        return qxGenErrToRefError(ReferenceErrorType::BallotBox, errorStatus);

    // Create elections from standard ballot box

    returnBuffer = electionTransform(bb, cc.seats());

    return ReferenceError();
}

/*!
 *  @param[out] returnBuffer A list of expected election results, filled
 *  with the provided data.
 *  @param[in] resultSetPath The path to the result set JSON file.
 *  @return An error object containing error details if the operation fails.
 */
ReferenceError expectedResultsFromReferenceInput(QList<ExpectedElectionResult>& returnBuffer,
                                                   const QString& resultSetPath)
{
    // Clear return buffer
    returnBuffer.clear();

    // Read file
    ResultSetReader rsReader(&returnBuffer, resultSetPath);
    return qxGenErrToRefError(ReferenceErrorType::ExpectedResult, rsReader.readInto());
}

/*!
 *  @param[out] returnBuffer A list of calculator options, filled
 *  with the provided data.
 *  @param[in] calcOptionsPath The path to the calculator option set file.
 *  @return An error object containing error details if the operation fails.
 */
ReferenceError calculatorOptionsFromReferenceInput(Star::Calculator::Options& returnBuffer,
                                                   const QString& calcOptionsPath)
{
    // Clear return buffer
    returnBuffer = Star::Calculator::Options();

    // Read file
    CalcOptionsReader opReader(&returnBuffer, calcOptionsPath);
    return qxGenErrToRefError(ReferenceErrorType::CalcOptions, opReader.readInto());
}

}
