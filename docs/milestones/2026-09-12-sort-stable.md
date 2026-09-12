# Intermediate milestone — sort stable / native RMB

Date: 2026-09-12

Branch state: `feature/cf-icons-sort-stack`

Checkpoint commit before icon redesign: `d05a3688da06f02be5fb89730525361dc935a1b8`

Validated by runtime testing at this point:

- Sort appears stable, including preservation of DayZ hand-slot reservations/placeholders.
- Native right-click behavior is restored; TransferZ no longer owns RMB drag or double-right-click gestures.
- `Shift + Left Drag` moves all direct items from the source container.
- `Alt + Left Drag` moves exact-class matches for the dragged item.
- Native stack splitting in hand routes the split result to `P*` when space is available, then falls back to vanilla DayZ behavior.
- Split stacks inside a container held in hands use source container -> `P*` -> vanilla fallback.

This milestone exists as a recovery/reference point before the header icon redesign. No merge to `main` is implied.
