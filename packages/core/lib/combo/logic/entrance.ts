import { cloneDeep } from 'lodash';

import { Random, shuffle } from '../random';
import { Settings } from '../settings';
import { DUNGEONS_REGIONS, ExprMap, ExprParsers, World, WorldEntrance } from './world';
import { Pathfinder } from './pathfind';
import { Monitor } from '../monitor';
import { LogicEntranceError, LogicError } from './error';
import { Expr, exprAnd, exprTrue } from './expr';
import { Game } from '../config';
import { makeLocation } from './locations';

export type EntranceShuffleResult = {
  overrides: Map<string, string>;
  boss: number[];
  dungeons: number[];
};

const BOSS_INDEX_BY_DUNGEON = {
  DT: 0,
  DC: 1,
  JJ: 2,
  Forest: 3,
  Fire: 4,
  Water: 5,
  Shadow: 6,
  Spirit: 7,
  WF: 8,
  SH: 9,
  GB: 10,
  IST: 11,
} as {[k: string]: number};

const DUNGEON_INDEX = {
  DT: 0,
  DC: 1,
  JJ: 2,
  Forest: 3,
  Fire: 4,
  Water: 5,
  Shadow: 6,
  Spirit: 7,
  WF: 8,
  SH: 9,
  GB: 10,
  IST: 11,
  ST: 12,
  SSH: 13,
  OSH: 14,
  BotW: 15,
  IC: 16,
  GTG: 17,
  BtW: 18,
  ACoI: 19,
  SS: 20,
  BtWE: 21,
  PF: 22,
  Ganon: 23,
  Tower: 24,
} as {[k: string]: number};;

export class LogicPassEntrances {
  private pathfinder: Pathfinder;
  private world: World;

  constructor(
    private readonly input: {
      world: World;
      exprParsers: ExprParsers;
      settings: Settings;
      random: Random;
      monitor: Monitor;
      attempts: number;
    },
  ) {
    this.world = cloneDeep(input.world);
    this.pathfinder = new Pathfinder(this.world, input.settings);
  }
  private result: EntranceShuffleResult = {
    overrides: new Map,
    boss: Object.values(BOSS_INDEX_BY_DUNGEON),
    dungeons: Object.values(DUNGEON_INDEX),
  };

  private getExpr(original: string) {
    const entrance = this.world.entrances.get(original)!;
    const from = this.input.world.areas[entrance.from];
    return from.exits[entrance.to];
  }

  private isAssignableNew(original: string, replacement: string, opts?: { ownGame?: boolean, locations?: string[] }) {
    const originalEntrance = this.world.entrances.get(original)!;
    const replacementEntrance = this.world.entrances.get(replacement)!;
    const dungeon = this.input.world.areas[replacementEntrance.to].dungeon!;

    /* Reject wrong game */
    if (opts?.ownGame) {
      if (originalEntrance.game !== replacementEntrance.game) {
        return false;
      }
    }

    /* Beatable only */
    if (this.input.settings.logic === 'beatable') {
      return true;
    }

    /* Apply an override */
    this.world.areas[originalEntrance.from].exits[replacementEntrance.to] = this.getExpr(original);

    /* If the dungeon is ST or IST, we need to allow the other dungeon */
    if (dungeon === 'ST') {
      this.world.areas['OOT SPAWN'].exits['MM Stone Tower Temple Inverted'] = exprTrue();
    }
    if (dungeon === 'IST') {
      this.world.areas['OOT SPAWN'].exits['MM Stone Tower Temple'] = exprTrue();
    }

    /* Check if the new world is valid */
    const pathfinderState = this.pathfinder.run(null, { singleWorld: true, ignoreItems: true, recursive: true });

    /* Restore the override */
    delete this.world.areas[originalEntrance.from].exits[replacementEntrance.to]
    delete this.world.areas['OOT SPAWN'].exits['MM Stone Tower Temple Inverted'];
    delete this.world.areas['OOT SPAWN'].exits['MM Stone Tower Temple'];

    /* Get the list of required locations */
    let locations: string[];
    if (opts?.locations) {
      locations = opts.locations;
    } else if (['ST', 'IST'].includes(dungeon)) {
      locations = [...this.world.dungeons['ST'], ...this.world.dungeons['IST']];
    } else {
      locations = Array.from(this.world.dungeons[dungeon]);
    }

    /* Turn into world 0 locations */
    const worldLocs = locations.map(l => makeLocation(l, 0));

    /* Check if the new world is valid */
    if (!(worldLocs.every(l => pathfinderState.locations.has(l))))
      return false;

    /* Ganon's tower check */
    if (dungeon === 'Tower' && ['ganon', 'both'].includes(this.input.settings.goal) && !pathfinderState.ws[0].events.has('OOT_GANON'))
      return false;

    return true;
  }

  private fixBosses() {
    const bossEntrances = [...this.world.entrances.values()].filter(e => e.type === 'boss');
    const bossEntrancesByDungeon = new Map<string, WorldEntrance>();
    const bossEvents = new Map<string, ExprMap>();
    const bossAreas = new Map<string, string[]>();
    const bossLocations = new Map<string, string[]>();

    /* Collect every boss event */
    for (const a in this.world.areas) {
      const area = this.world.areas[a];
      const dungeon = area.dungeon!;
      if (area.boss) {
        /* Events */
        if (!bossEvents.has(dungeon)) {
          bossEvents.set(dungeon, {});
        }
        const oldEvents = bossEvents.get(dungeon)!;
        bossEvents.set(dungeon, { ...oldEvents, ...area.events });

        /* Areas */
        if (!bossAreas.has(dungeon)) {
          bossAreas.set(dungeon, []);
        }
        bossAreas.get(dungeon)!.push(a);

        /* Locations */
        if (!bossLocations.has(dungeon)) {
          bossLocations.set(dungeon, []);
        }
        const locs = bossLocations.get(dungeon)!;
        for (const l in area.locations) {
          locs.push(l);
        }

        /* Remove the event */
        area.events = {};
      }
    }

    /* Collect the entrances and delete the original ones */
    for (const e of bossEntrances) {
      const areaFrom = this.world.areas[e.from];
      const areaTo = this.world.areas[e.to];

      /* We have a boss entrance */
      const dungeon = areaTo.dungeon!;
      bossEntrancesByDungeon.set(dungeon, e);

      /* Remove the entrance */
      delete areaFrom.exits[e.to];
    }

    /* Actually shuffle bosses */
    const bosses = shuffle(this.input.random, Array.from(bossEntrancesByDungeon.keys()));
    const bossesToPlace = new Set(bosses);
    for (const srcBoss of bosses) {
      const src = bossEntrancesByDungeon.get(srcBoss)!;
      const candidates = shuffle(this.input.random, Array.from(bossesToPlace));
      let dstBoss: string | null = null;
      for (const boss of candidates) {
        const dst = bossEntrancesByDungeon.get(boss)!;
        if (this.isAssignableNew(src.id, dst.id, { ownGame: this.input.settings.erBoss === 'ownGame', locations: bossLocations.get(boss)! })) {
          dstBoss = boss;
          break;
        }
      }
      if (dstBoss === null) {
        throw new LogicEntranceError(`Nowhere to place boss ${srcBoss}`);
      }
      bossesToPlace.delete(dstBoss);

      /* We found a boss - place it */
      const dst = bossEntrancesByDungeon.get(dstBoss)!;
      this.place(src.id, dst.id, false);

      /* Mark the boss */
      this.result.boss[BOSS_INDEX_BY_DUNGEON[dstBoss]] = BOSS_INDEX_BY_DUNGEON[srcBoss];

      /* Add the events */
      const areaNames = bossAreas.get(dstBoss)!;
      const lastAreaName = areaNames[areaNames.length - 1];
      const lastArea = this.world.areas[lastAreaName];
      lastArea.events = { ...lastArea.events, ...bossEvents.get(srcBoss)! };

      /* Change the associated dungeon */
      for (const a of bossAreas.get(dstBoss)!) {
        const area = this.world.areas[a];
        area.dungeon = srcBoss;

        for (const loc in area.locations) {
          this.world.regions[loc] = DUNGEONS_REGIONS[srcBoss];
          this.world.dungeons[dstBoss].delete(loc);
          this.world.dungeons[srcBoss].add(loc);
        }
      }
    }
  }

  private fixDungeons() {
    /* Set the dungeon list */
    let shuffledDungeons = new Set(['DT', 'DC', 'JJ', 'Forest', 'Fire', 'Water', 'Shadow', 'Spirit', 'WF', 'SH', 'GB', 'ST', 'IST']);
    if (this.input.settings.erMinorDungeons) {
      ['BotW', 'IC', 'GTG'].forEach(d => shuffledDungeons.add(d));
    }
    if (this.input.settings.erGanonCastle) {
      shuffledDungeons.add('Ganon');
    }
    if (this.input.settings.erGanonTower) {
      shuffledDungeons.add('Tower');
    }
    if (this.input.settings.erSpiderHouses) {
      ['SSH', 'OSH'].forEach(d => shuffledDungeons.add(d));
    }
    if (this.input.settings.erPirateFortress) {
      shuffledDungeons.add('PF');
    }
    if (this.input.settings.erBeneathWell) {
      ['BtW', 'BtWE'].forEach(d => shuffledDungeons.add(d));
    }
    if (this.input.settings.erIkanaCastle) {
      shuffledDungeons.add('ACoI');
    }
    if (this.input.settings.erSecretShrine) {
      shuffledDungeons.add('SS');
    }

    /* Get the transitions and exprs */
    const dungeonTransitions = [...this.world.entrances.values()]
      .filter(e => e.type === 'dungeon')
      .filter(e => shuffledDungeons.has(this.world.areas[e.from].dungeon!) || shuffledDungeons.has(this.world.areas[e.to].dungeon!));

    const dungeonEntrances = new Map<string, WorldEntrance>();

    for (const e of dungeonTransitions) {
      /* Get the exit */
      const exit = this.world.entrances.get(e.reverse!)!;

      /* Get the various areas */
      const entranceAreaFrom = this.world.areas[e.from];
      const entranceAreaTo = this.world.areas[e.to];
      const exitAreaFrom = this.world.areas[exit.from];

      /* Save the entrance */
      dungeonEntrances.set(entranceAreaTo.dungeon!, e);

      /* Remove the transitions */
      delete entranceAreaFrom.exits[e.to];
      delete exitAreaFrom.exits[exit.to];
    }

    /* Assign the dungeons */
    const dungeons = shuffle(this.input.random, Array.from(shuffledDungeons));
    const dungeonsDest = new Set(dungeons);
    while (dungeons.length > 0) {
      const dungeon = dungeons.pop()!;
      const candidates = shuffle(this.input.random, Array.from(dungeonsDest));
      let destDungeon: string | null = null;

      for (const c of candidates) {
        const src = dungeonEntrances.get(dungeon)!.id;
        const dst = dungeonEntrances.get(c)!.id;
        const assignable = this.isAssignableNew(src, dst, { ownGame: this.input.settings.erDungeons === 'ownGame' });
        if (assignable) {
          destDungeon = c;
          break;
        }
      }

      if (!destDungeon) {
        throw new LogicError('Unable to assign a dungeon to location: ' + dungeon);
      }

      /* Change the world */
      dungeonsDest.delete(destDungeon);
      const sourceEntranceId = dungeonEntrances.get(dungeon)!.id;
      const destEntranceId = dungeonEntrances.get(destDungeon)!.id;

      this.place(sourceEntranceId, destEntranceId, false);

      /* Store the dungeon */
      this.result.dungeons[DUNGEON_INDEX[destDungeon]] = DUNGEON_INDEX[dungeon];
    }
  }

  private songOfTime(e: Expr): Expr {
    const subcond = this.input.exprParsers.mm.parse('can_reset_time');
    return exprAnd([e, subcond]);
  }

  private placeSingle(original: string, replacement: string, overworld: boolean) {
    const entranceOriginal = this.world.entrances.get(original)!;
    const entranceReplacement = this.world.entrances.get(replacement)!;

    /* Change the world */
    let expr = this.getExpr(original);
    if (overworld && entranceOriginal.game === 'oot' && entranceReplacement.game === 'mm') {
      this.world.areas[entranceOriginal.from].exits['MM GLOBAL'] = expr;
      expr = this.songOfTime(expr);
    }
    this.world.areas[entranceOriginal.from].exits[entranceReplacement.to] = expr;

    /* Mark the override */
    this.result.overrides.set(original, replacement);
  }

  private place(original: string, replacement: string, overworld: boolean) {
    const entranceOriginal = this.world.entrances.get(original)!;
    const entranceReplacement = this.world.entrances.get(replacement)!;

    this.placeSingle(original, replacement, overworld);
    if (entranceOriginal.reverse && entranceReplacement.reverse) {
      this.placeSingle(entranceReplacement.reverse, entranceOriginal.reverse, overworld);
    }
  }

  private placeRegions() {
    /* Get overworld entrances */
    const entrances = [...this.world.entrances.values()].filter(e => e.type === 'region');

    /* Delete the overworld entrances from the world */
    for (const e of entrances) {
      delete this.world.areas[e.from].exits[e.to];
    }

    /* Shuffle the entrances */
    const shuffledEntrances = shuffle(this.input.random, entrances);

    /* Apply the entrances */
    for (let i = 0; i < entrances.length; ++i) {
      this.place(entrances[i].id, shuffledEntrances[i].id, true);
    }
  }

  run() {
    this.input.monitor.log(`Logic: Entrances (attempt ${this.input.attempts})`);

    if (this.input.settings.erRegions) {
      this.placeRegions();
    }

    if (this.input.settings.erDungeons !== 'none') {
      this.fixDungeons();
    }

    if (this.input.settings.erBoss !== 'none') {
      this.fixBosses();
    }

    return { world: this.world, entrances: this.result };
  }
};
