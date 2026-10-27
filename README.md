# SchemaEntity (mrc4tt fork)

Schema-based entity wrappers for CS2 Metamod:Source plugins. `SCHEMA_FIELD` resolves field
offsets at runtime from the game's `CSchemaSystem`, so they keep working across game updates.
Fork of [Pisex/SchemaEntity](https://github.com/Pisex/SchemaEntity).

## Compatibility

Builds against [mrc4tt/hl2sdk](https://github.com/mrc4tt/hl2sdk) (`cs2` branch, s2sdk layout)
and Metamod:Source 2.x. It is not compatible with the old hl2sdk layout (`tier1/`, old schema
struct names). Use upstream for that.

Changes from upstream:

- `tier1/utlmap.h` include moved to `utlmap.h`.
- Schema struct members renamed: `m_nNumBaseClasses`, `m_nNumFields`, `m_nNumMetadata`,
  `m_pMetadata`, `m_pName`.
- `NetworkStateChanged_t` changed to `NetworkStateChangedData`.
- `globaltypes.h` includes `gametrace.h`.
- `FL_FAKECLIENT` / `FL_BOT` flag names.

## Usage

Add as a submodule. Keep the folder name lowercase, because AMBuild lowercases include paths
and MSVC builds under Wine run on a case-sensitive filesystem:

```bash
git submodule add https://github.com/mrc4tt/SchemaEntity schemaentity
```

Then, in `AMBuilder`:

```python
binary.compiler.cxxincludes += [os.path.join(builder.sourcePath, 'schemaentity')]
binary.sources += [os.path.join('schemaentity', 'schemasystem.cpp')]
```

## Caveat

Some wrappers call virtual functions by hardcoded vtable index (`ChangeTeam`, `Respawn`,
`CommitSuicide`, `DropWeapon`, `RemoveWeapons`). These indices can change when CS2 updates.
Check them before you use them.

## Credits

[Pisex](https://github.com/Pisex): original SchemaEntity.
