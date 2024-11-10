/**
 * @file prog01/app/src/audio/score.c
 *
 * 譜面制御
 *
 * @date 2024.11.10 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/audio/score.h>

#include <user/types.h>

#include <stdbool.h>
#include <stdint.h>

#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

UError_t AudioScore_Create(AudioScore_t* score, const AudioNote_t* notes, uint32_t numNotes) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == score) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    score->notes = notes;
    score->numNotes = numNotes;
    score->scorePos = 0u;
    score->notePos = 0u;
  }

  return err;
}

UError_t AudioScore_Init(AudioScore_t* score) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == score) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    score->scorePos = 0u;
    score->notePos = 0u;
  }

  return err;
}

UError_t AudioScore_Step(AudioScore_t* score) {
  UError_t err = uSuccess;

  const uint32_t next = score->notePos + 1;
  if (next >= score->notes[score->scorePos].length) {
    score->scorePos++;
    score->notePos = 0;
  } else {
    score->notePos = next;
  }

  return err;
}

bool AudioScore_IsFinished(const AudioScore_t* score) {
  UError_t err = uSuccess;
  bool ret = false;

  if (uSuccess == err) {
    if (NULL == score) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret = (score->scorePos >= score->numNotes);
  }

  return ret;
}

bool AudioScore_IsNoteChanged(const AudioScore_t* score) {
  UError_t err = uSuccess;
  bool ret = false;

  if (uSuccess == err) {
    if (NULL == score) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret = (0u == score->notePos);
  }

  return ret;
}

UError_t AudioScore_GetNote(const AudioScore_t* score, AudioNote_t* note) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == score || NULL == note || NULL == score->notes) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (AudioScore_IsFinished(score)) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    memcpy(note, &score->notes[score->scorePos], sizeof(AudioNote_t));
  }

  return err;
}
