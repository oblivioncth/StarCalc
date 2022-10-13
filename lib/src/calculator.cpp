// Unit Include
#include "star/calculator.h"

// Qx Includes
#include <qx/core/qx-string.h>

namespace Star
{

//===============================================================================================================
// Calculator
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------------
//Public:
Calculator::Calculator() :
    mElection(nullptr),
    mExtraTiebreak(false)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
QStringList Calculator::determinePreliminaryLeaders()
{
    emit calculationDetail(LOG_EVENT_DETERMINE_PRELIMINARY_LEADERS);

    QStringList leaders;

    // Handle tie & non-tie cases
    QStringList nomineesInFirst = mElection->scoreRankings().front().nominees;
    if(nomineesInFirst.size() > 1) // First place tie
    {
        uint firstPlaceScore = mElection->scoreRankings().front().value;

        if(nomineesInFirst.size() == 2) // Two-way
        {
            QString tieLogStr = LOG_EVENT_PRELIMINARY_FIRST_TIE_BENIGN.arg(firstPlaceScore) + '\n' + createNomineeGeneralListString(nomineesInFirst);
            emit calculationDetail(tieLogStr);

            leaders = nomineesInFirst;
        }
        else // N-way
        {
            QString tieLogStr = LOG_EVENT_PRELIMINARY_FIRST_TIE.arg(nomineesInFirst.size()).arg(firstPlaceScore) + '\n' + createNomineeGeneralListString(nomineesInFirst);
            emit calculationDetail(tieLogStr);

            // Tiebreak
            QPair<QStringList, QStringList> firstPlaceTiebreak = breakScoreTie(nomineesInFirst);

            // Need at least 2 nominees
            leaders.append(firstPlaceTiebreak.first);
            if(leaders.size() < 2)
                leaders.append(firstPlaceTiebreak.second); // If there are ties for second pref rank, they all just go on
        }
    }
    else
    {
        // First place uncontested
        QString first = nomineesInFirst.front();
        emit calculationDetail(LOG_EVENT_PRELIMINARY_FIRST_NO_TIE.arg(first));
        leaders.append(first);

        // Check second place
        QStringList nomineesInSecond = mElection->scoreRankings().at(1).nominees;
        if(nomineesInSecond.size() > 1) // Second place tie
        {
            uint secondPlaceScore = mElection->scoreRankings().at(1).value;

            QString tieLogStr = LOG_EVENT_PRELIMINARY_SECOND_TIE.arg(nomineesInSecond.size()).arg(secondPlaceScore) + '\n' + createNomineeGeneralListString(nomineesInSecond);
            emit calculationDetail(tieLogStr);

            // Tiebreak
            QPair<QStringList, QStringList> secondPlaceTiebreak = breakScoreTie(nomineesInSecond);

            // Append all in first place of preference rankings, ties or not
            leaders.append(secondPlaceTiebreak.first);
        }
        else
        {
            // Second place uncontested
            QString second = nomineesInSecond.front();
            emit calculationDetail(LOG_EVENT_PRELIMINARY_SECOND_NO_TIE.arg(second));
            leaders.append(second);
        }
    }

    emit calculationDetail(LOG_EVENT_PRELIMINARY_LEADERS + '\n' + createNomineeToalScoreListString(leaders));
    return leaders;
}

QPair<QStringList, QStringList> Calculator::performPrimaryRunoff(const QStringList& preliminaryLeaders)
{
    emit calculationDetail(LOG_EVENT_PERFORM_PRIMARY_RUNOFF);

    // Buffers
    QStringList winners;
    QStringList runnerUps;

    // Determine preference ranking
    emit calculationDetail(LOG_EVENT_PRIMARY_PREF_RANK_SORT);
    QList<Rank> prefRanks = rankByPreference(preliminaryLeaders);

    // Handle tie & non-tie cases
    QStringList nomineesInFirst = prefRanks.front().nominees;
    if(nomineesInFirst.size() > 1) // First place tie
    {
        uint firstPlacePrefCount = prefRanks.front().value;
        QString tieLogStr = LOG_EVENT_PRIMARY_FIRST_TIE.arg(nomineesInFirst.size()).arg(firstPlacePrefCount) + '\n' + createNomineeGeneralListString(nomineesInFirst);
        emit calculationDetail(tieLogStr);

        // Tiebreak
        QPair<QStringList, QStringList> firstPlaceTiebreak = breakPreferenceTie(nomineesInFirst);

        // Determine winners/runner-ups
        winners = firstPlaceTiebreak.first;
        if(!firstPlaceTiebreak.second.isEmpty())
            runnerUps = firstPlaceTiebreak.second;
        else
        {
            emit calculationDetail(LOG_EVENT_PRIMARY_TIEBREAK_FAIL);

            // Get second place from preference rankings, if available
            if(prefRanks.size() > 1)
            {
                QStringList nomineesInSecond = prefRanks.at(1).nominees;
                runnerUps = nomineesInSecond.size() > 1 ? breakPreferenceTie(nomineesInSecond).first : nomineesInSecond;
            }
            else
                emit calculationDetail(LOG_EVENT_PRIMARY_TIEBREAK_FAIL_NO_FALLBACK);
        }
    }
    else
    {
        // First place uncontested
        QString first = nomineesInFirst.front();
        emit calculationDetail(LOG_EVENT_PRIMARY_FIRST_NO_TIE.arg(first));
        winners = {first};

        // Check second place
        QStringList nomineesInSecond = prefRanks.at(1).nominees;
        if(nomineesInSecond.size() > 1) // Second place tie
        {
            uint secondPlacePrefCount = prefRanks.at(1).value;
            QString tieLogStr = LOG_EVENT_PRIMARY_SECOND_TIE.arg(nomineesInSecond.size()).arg(secondPlacePrefCount) + '\n' + createNomineeGeneralListString(nomineesInSecond);
            emit calculationDetail(tieLogStr);

            // Tiebreak
            QPair<QStringList, QStringList> secondPlaceTiebreak = breakPreferenceTie(nomineesInSecond);
            runnerUps = secondPlaceTiebreak.first;
        }
        else
        {
            // Second place uncontested
            QString second = nomineesInSecond.front();
            emit calculationDetail(LOG_EVENT_PRIMARY_SECOND_NO_TIE.arg(second));
            runnerUps = {second};
        }
    }

    // Note initial results
    emit calculationDetail(LOG_EVENT_INITIAL_RESULT_WINNERS.arg(winners.join(',')));
    emit calculationDetail(LOG_EVENT_INITIAL_RESULT_RUNNERUPS.arg(runnerUps.join(',')));

    // Return result
    return {winners, runnerUps};
}
QPair<QStringList, QStringList> Calculator::performExtendedTiebreak(QStringList winners, QStringList runnerUps)
{
    // If there is still a tie, check effect of extended tiebreak
    if(winners.size() > 1)
    {
        emit calculationDetail(LOG_EVENT_INITIAL_RESULT_WINNER_TIE);

        // Tiebreak
        QPair<QStringList, QStringList> firstPlaceTiebreak = breakExtendedTie(winners);

        // Determine winners/runner-ups
        winners = firstPlaceTiebreak.first;
        if(!firstPlaceTiebreak.second.isEmpty())
            runnerUps = firstPlaceTiebreak.second;
        else
        {
            emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_FAIL);

            // Get second place from original second place
            runnerUps = runnerUps.size() > 1 ? breakExtendedTie(runnerUps).first : runnerUps;
        }

    }
    else if(runnerUps.size() > 1)
    {
        emit calculationDetail(LOG_EVENT_INITIAL_RESULT_RUNNERUP_TIE);

        // Tiebreak
        QPair<QStringList, QStringList> secondPlaceTiebreak = breakExtendedTie(winners);
        runnerUps = secondPlaceTiebreak.first;
    }
    else
        emit calculationDetail(LOG_EVENT_INITIAL_RESULT_NO_TIE);

    // Note results
    emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_WINNERS.arg(winners.join(',')));
    emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_RUNNERUPS.arg(runnerUps.join(',')));

    // Return the result
    return {winners, runnerUps};
}

QList<Rank> Calculator::rankByPreference(const QStringList& nominees)
{
    // Determine aggregate preference of nominee list
    emit calculationDetail(LOG_EVENT_RANK_BY_PREF);
    QMap<QString, uint> totalPreferenceMap;

    /* Add all nominees with an initial preference count of 0, since some might not be preferred even
     * once, and therefore be missed by the next loop
     */
    for(const QString& nominee : nominees)
        totalPreferenceMap[nominee] = 0;

    for(const Star::Election::Ballot& ballot : mElection->ballots())
    {
        QString voterName = ballot.voter().anonymousName;
        QString pref = ballot.preference(nominees);
        if(!pref.isNull())
        {
            uint newTotal = ++totalPreferenceMap[pref];
            emit calculationDetail(LOG_EVENT_RANK_BY_PREF_HAS_PREF.arg(voterName, pref).arg(newTotal));
        }
        else
            emit calculationDetail(LOG_EVENT_RANK_BY_PREF_NO_PREF.arg(voterName));
    }

    // Create sorted rank list
    QList<Rank> prefRanks =  Rank::rankSort(totalPreferenceMap);

    emit calculationDetail(LOG_EVENT_RANKINGS_PREF + '\n' + createNomineeRankListString(prefRanks));
    return prefRanks;
}

QList<Rank> Calculator::rankByScore(const QStringList& nominees)
{
    /* Determine aggregate score of nominee list
     * Redoing this with the provided sub-list is more straight forward than trying to manipulate
     * the full score rankings that are part of the Election
     */
    emit calculationDetail(LOG_EVENT_RANK_BY_SCORE);
    QMap<QString, uint> totalScoreMap;

    for(const QString& nominee : nominees)
        totalScoreMap[nominee] = mElection->totalScore(nominee);

    // Create sorted rank list
    QList<Rank> scoreRanks = Rank::rankSort(totalScoreMap);

    emit calculationDetail(LOG_EVENT_RANKINGS_SCORE + '\n' + createNomineeRankListString(scoreRanks));
    return scoreRanks;
}

QList<Rank> Calculator::rankByVotesOfMaxScore(const QStringList& nominees)
{
    // Determine aggregate max votes of nominee list
    emit calculationDetail(LOG_EVENT_RANK_BY_VOTES_OF_MAX_SCORE);
    QMap<QString, uint> totalMaxVotesMap;

    for(const QString& nominee : nominees)
    {
        uint maxVoteCount = 0;

        for(const Star::Election::Ballot& ballot : mElection->ballots())
            if(ballot.score(nominee) == 5)
                maxVoteCount++;

        totalMaxVotesMap[nominee] = maxVoteCount;
    }

    // Create sorted rank list
    QList<Rank> maxVoteRanks = Rank::rankSort(totalMaxVotesMap);

    emit calculationDetail(LOG_EVENT_RANKINGS_VOTES_OF_MAX_SCORE + '\n' + createNomineeRankListString(maxVoteRanks));
    return maxVoteRanks;
}

QPair<QStringList, QStringList> Calculator::breakScoreTie(const QStringList& nominees)
{
    // Check number of times a nominee is preferred to break tie
    emit calculationDetail(LOG_EVENT_BREAK_SCORE_TIE.arg(nominees.size()));
    QList<Rank> prefRanks = rankByPreference(nominees);
    QPair<QStringList, QStringList> tieBreak(prefRanks.front().nominees, prefRanks.size() > 1 ? prefRanks.at(1).nominees : QStringList());

    emit calculationDetail(LOG_EVENT_BREAK_RESULT.arg(tieBreak.first.join(','), tieBreak.second.join(',')));
    return tieBreak;
}

QPair<QStringList, QStringList> Calculator::breakPreferenceTie(const QStringList& nominees)
{
    // Check score to break tie
    emit calculationDetail(LOG_EVENT_BREAK_PREF_TIE.arg(nominees.size()));
    QList<Rank> scoreRanks = rankByScore(nominees);
    QPair<QStringList, QStringList> tieBreak(scoreRanks.front().nominees, scoreRanks.size() > 1 ? scoreRanks.at(1).nominees : QStringList());

    emit calculationDetail(LOG_EVENT_BREAK_RESULT.arg(tieBreak.first.join(','), tieBreak.second.join(',')));
    return tieBreak;
}

QPair<QStringList, QStringList> Calculator::breakExtendedTie(const QStringList& nominees)
{
    // Check number of times a nominee was given the maximum score possible to break tie
    emit calculationDetail(LOG_EVENT_BREAK_EXTENDED_TIE.arg(nominees.size()));
    QList<Rank> maxVoteRanks = rankByVotesOfMaxScore(nominees);
    QPair<QStringList, QStringList> tieBreak(maxVoteRanks.front().nominees, maxVoteRanks.size() > 1 ? maxVoteRanks.at(1).nominees : QStringList());

    emit calculationDetail(LOG_EVENT_BREAK_RESULT.arg(tieBreak.first.join(','), tieBreak.second.join(',')));
    return tieBreak;
}

QString Calculator::createNomineeGeneralListString(const QStringList& nominees)
{
    if(nominees.isEmpty())
        return QString();

    return Qx::String::join(nominees, "\n", "\t- ");
}

QString Calculator::createNomineeToalScoreListString(const QStringList& nominees)
{
    if(nominees.isEmpty())
        return QString();

    QString listStr;
    qsizetype lastIdx = nominees.size() - 1;
    for(qsizetype i = 0; i < nominees.size(); i++)
    {
        const QString& nominee = nominees.at(i);
        QString nomineeStr = LIST_ITEM_NOMINEE_TOTAL_SCORE.arg(nominee).arg(mElection->totalScore(nominee));
        if(i != lastIdx)
            nomineeStr += '\n';
        listStr.append(nomineeStr);
    }

    return listStr;
}

QString Calculator::createNomineeRankListString(const QList<Rank>& ranks)
{
    if(ranks.isEmpty())
        return QString();

    QString listStr;
    qsizetype lastIdx = ranks.size() - 1;
    for(qsizetype i = 0; i < ranks.size(); i++)
    {
        const Rank& rank = ranks.at(i);
        QString nomineeStr = LIST_ITEM_RANK.arg(i).arg(rank.nominees.join(R"(", ")")).arg(rank.value);
        if(i != lastIdx)
            nomineeStr += '\n';
        listStr.append(nomineeStr);
    }

    return listStr;
}

//Public:
bool Calculator::isExtraTiebreak() const { return mExtraTiebreak; }
const Election* Calculator::election() const { return mElection; }
void Calculator::setElection(const Election* election) { mElection = election; }
void Calculator::setExtraTiebreak(bool extraTiebreak) { mExtraTiebreak = extraTiebreak; }

ElectionResult Calculator::calculateResult()
{
    // Check for valid election
    if(!mElection || !mElection->isValid())
    {
        emit calculationDetail(LOG_EVENT_INVALID_ELECTION);
        return ElectionResult();
    }

    // Log start
    emit calculationDetail(LOG_EVENT_CALC_START.arg(mElection->name()));

    // Note counts
    emit calculationDetail(LOG_EVENT_INPUT_COUNTS.arg(mElection->nominees().size()).arg(mElection->ballots().size()));

    // Print out raw score rankings
    emit calculationDetail(LOG_EVENT_INITAL_RAW_RANKINGS + '\n' + createNomineeRankListString(mElection->scoreRankings()));

    // Determine preliminary leaders based on raw score
    QStringList preliminaryLeaders = determinePreliminaryLeaders();

    // Perform primary runoff
    QPair<QStringList, QStringList> results = performPrimaryRunoff(preliminaryLeaders);

    // Check if extended tiebreak could matter
    if(results.first.size() > 1 || results.second.size() > 1)
    {
        QPair<QStringList, QStringList> extendedResults = performExtendedTiebreak(results.first, results.second);

        if(extendedResults == results)
            emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_IRRELAVENT);
        else if(mExtraTiebreak)
        {
            emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_ENABLED);
            results = extendedResults;
        }
        else
            emit calculationDetail(LOG_EVENT_EXTENDED_TIEBREAK_DISABLED);
    }

    // Note final results
    emit calculationDetail(LOG_EVENT_FINAL_RESULT_WINNERS.arg(results.first.join(',')));
    emit calculationDetail(LOG_EVENT_FINAL_RESULT_RUNNERUPS.arg(results.second.join(',')));

    // Return final results
    return ElectionResult(mElection, results.first, results.second);
}


}
