# Outfit Gallery 1.0.13 — 検証記録 / Validation

2026-10-09、公開用DLLメタデータ1.0.13.0。

## ビルド・配布確認

- Windows x64 / MSVC / RelWithDebInfoでクリーンビルド成功。その後ログ内の旧版番号を整理して再ビルド成功。
- 公開用ソースで28/28テスト成功。設定永続化・翻訳テンプレート・装備範囲・編集・クラフトセッション等を含む。
- 最新試験版layout-test1（1.0.12.44）との差分は、版番号とログの識別文字列、説明書・配布構成のみ。全srcファイルを比較し、実処理が変わっていないことを確認。
- ランタイムZIPにはDLL・初期INI・説明書・翻訳テンプレート・ライセンスを含む。ユーザー写真、保存済み設定JSON、セーブ、ローカルログは含めない。
- ZIPのCRC、DLLの元ビルドとの一致、ソースZIPの元ソースとの一致、SHA256を確認。

## 実機で報告された範囲

- 自動着替えの操作、屋内のお店、オプション整理、ON/OFFショートカットについて利用者の確認あり。
- 鍛冶：自動と手動の同一装備で表示結果が一致（34／上質／58）。先行する逆転は比較元の「全部入り」装備の差と判明。
- 錬金：台での着替え発動とIED上の装備効果を確認。完成した薬の性能比較は実施していない。
- 付呪：台での着替え発動、アージダル装備効果とスキル18→28を確認。完成品の性能比較は実施していない。
- スロットなし小物の登録は確認済み。個別交換の修正後の装着・解除の独立した詳細報告はなし。範囲と保存の自動テストは通過。
- 最新の装備管理一覧の拡張は利用者から「使いやすくなった」「概ね良さそう」と報告。

新しい公開版番号のDLLは、この作業ではゲームに配置していない。1.0.13そのものの起動確認と、全実行ファイル・全MOD構成での網羅的な再検証は未実施。錬金・付呪終了後の復帰は独立した実機報告がない。対応対象と検証対象の違いはREADMEに記載。

Release build and all 28 tests passed. Production logic matches layout-test1; only version/log identifiers changed. Gameplay evidence comes from the preceding trial builds. The newly numbered release has not been deployed or launched in-game here. Crafting remains experimental; alchemy/enchanting evidence covers equipment/effect application, not crafted-output strength.
