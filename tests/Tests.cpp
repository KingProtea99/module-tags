// Tags module tests: stable ids whatever the input order, parent chains, counted grants, "can act" checks.
#include "tags/sim/Tags.h"

#include "mealkit/core/Hash.h"

#include <cstdio>

static int g_failed = 0;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            std::printf("%s(%d): FAIL %s\n", __FILE__, __LINE__, #x);                                                  \
            ++g_failed;                                                                                                \
        }                                                                                                              \
    } while (0)

int main() {
    const char* a[] = {"State.Stunned.Frozen", "Ability.Fire", "State.Dead", "State.Stunned"};
    const char* b[] = {"State.Dead", "State.Stunned", "Ability.Fire", "State.Stunned.Frozen"};
    tags::Registry ra, rb;
    CHECK(ra.build(a, 4));
    CHECK(rb.build(b, 4));
    // Same ids from any input order; parents were interned and sort first
    CHECK(ra.count() == 6); // Ability, Ability.Fire, State, State.Dead, State.Stunned, State.Stunned.Frozen
    for (const char* n : {"Ability", "Ability.Fire", "State", "State.Dead", "State.Stunned", "State.Stunned.Frozen"})
        CHECK(ra.find(n) != tags::kNone && ra.find(n) == rb.find(n));
    CHECK(ra.find("Ability") == 1 && ra.find("State.Stunned.Frozen") == 6);
    CHECK(ra.find("Nope") == tags::kNone);
    CHECK(ra.parent(ra.find("State.Stunned")) == ra.find("State"));
    CHECK(ra.parent(ra.find("State")) == tags::kNone);
    CHECK(ra.isUnder(ra.find("State.Stunned.Frozen"), ra.find("State")));
    CHECK(!ra.isUnder(ra.find("State"), ra.find("State.Stunned")));
    CHECK(!ra.isUnder(ra.find("State"), tags::kNone));

    // Counted grants include parents; two sources need two removes
    const auto state = ra.find("State"), stunned = ra.find("State.Stunned"), dead = ra.find("State.Dead");
    tags::TagCounts t;
    tags::add(t, ra, stunned);
    tags::add(t, ra, stunned);
    CHECK(tags::has(t, stunned) && tags::has(t, state) && !tags::has(t, dead));
    tags::remove(t, ra, stunned);
    CHECK(tags::has(t, stunned));
    tags::remove(t, ra, stunned);
    CHECK(!tags::has(t, stunned) && !tags::has(t, state));
    tags::remove(t, ra, stunned); // never underflows
    CHECK(!tags::has(t, state));

    // Can-act check
    const mealkit::u16 fire[] = {ra.find("Ability.Fire")}, blocked[] = {stunned, dead};
    tags::add(t, ra, fire[0]);
    CHECK(tags::allows(t, fire, 1, blocked, 2));
    tags::add(t, ra, dead);
    CHECK(!tags::allows(t, fire, 1, blocked, 2));
    CHECK(!tags::allows(t, fire, 0, blocked, 2)); // dead blocks even with no requirements
    CHECK(tags::allows(t, fire, 1, blocked, 0));

    // Bad input: empty segment, too long, too many tags
    tags::Registry bad;
    const char* empty[] = {"A..B"};
    CHECK(!bad.build(empty, 1) && bad.count() == 0);
    char many[tags::kMaxTags + 2][8];
    const char* ptrs[tags::kMaxTags + 2];
    for (unsigned i = 0; i < tags::kMaxTags + 2; ++i) {
        std::snprintf(many[i], sizeof many[i], "T%u", i);
        ptrs[i] = many[i];
    }
    CHECK(!bad.build(ptrs, tags::kMaxTags + 2));

    // State goes into the hash
    tags::TagCounts x, y;
    tags::add(x, ra, dead);
    tags::add(y, ra, dead);
    mealkit::Hasher hx, hy, hz;
    mealkit::reflect::hashStruct(mealkit::reflect::typeOf<tags::TagCounts>(), &x, hx);
    mealkit::reflect::hashStruct(mealkit::reflect::typeOf<tags::TagCounts>(), &y, hy);
    CHECK(hx.value == hy.value);
    tags::add(y, ra, stunned);
    mealkit::reflect::hashStruct(mealkit::reflect::typeOf<tags::TagCounts>(), &y, hz);
    CHECK(hx.value != hz.value);

    std::printf(g_failed ? "tags tests: %d failed\n" : "tags tests: all passed\n", g_failed);
    return g_failed ? 1 : 0;
}
