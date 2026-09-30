// UI translation only: stable ### IDs keep focus and open menus intact.
int language=0; // 0 English, 1 Japanese
const std::map<std::string,std::string> japanese{
 {"Maintain this follower's outfit","この服装を維持"},
 {"Restore standard outfit","標準装備に戻す"},
 {"Disables maintenance and restores standard armor. Given items and weapons are kept.","維持を解除して標準の防具を着せ直します。渡した所持品と武器は残します。"},
 {"Follower outfit maintenance enabled.","このフォロワーの服装維持を有効にしました。"},
 {"Follower outfit maintenance disabled.","このフォロワーの服装維持を解除しました。"},
 {"Follower is not available for outfit maintenance.","フォロワーの状態により服装維持・標準装備の復帰を行えません。"},
 {"No standard outfit is defined for this follower.","このフォロワーには標準衣装が設定されていません。服装維持は解除しました。"},
 {"Standard outfit uses leveled items; this feature cannot safely restore it.","標準衣装がレベル別抽選装備を含むため、この機能では安全に復帰できません。服装維持は解除しました。"},
 {"No armor was found in this follower's standard outfit.","標準衣装に防具が見つかりません。服装維持は解除しました。"},
 {"Checks every 5 seconds while closed. Missing items or repeated conflicts disable maintenance.","画面を閉じている間、約5秒ごとに確認します。装備の紛失や競合の繰り返しで維持を解除します。"},
 {"Select follower under crosshair (experimental)","クロスヘアのフォロワーを選択（試験機能）"},
 {"Follower: %s","対象フォロワー：%s"},
 {"Target: Player","対象：プレイヤー"},
 {"Studio closed: target is no longer available.","対象の状態が変化したため、画面を閉じました。"},
 {"Aim at a humanoid current follower, or look away to select yourself.","同行中の人型フォロワーに照準を合わせてください。自分を選ぶ場合は照準を外してください。"},
 {"The follower must be alive, loaded, standing and out of combat.","フォロワーが生存し、立っていて戦闘中でない状態で起動してください。"},
 {"Experimental: Other mods may change the outfit.","試用機能：他のMOD等により服装が変わる場合があります。"},
 {"Close the gallery, aim at a current humanoid follower and press the opening hotkey. Look away to select yourself. Target is fixed until close.","一度閉じ、同行中の人型フォロワーに照準を合わせて起動してください。照準を外すと自分を選択します。対象は閉じるまで固定されます。"},
 {"Independent preview","独立プレビュー"},
 {"Live preview","ライブプレビュー"},
 {"Reset preview position","プレビュー位置をリセット"},
 {"Drag the preview title to move it; drag its corner to resize. Photos use the centered view. Effects depend on your graphics setup.","タイトルをドラッグして移動、枠の隅でサイズ変更。中央の映像を写真に保存します。エフェクトの見え方は描画環境に依存します。"},
 {"Opening tab","起動時のタブ"},
 {"Choose the tab shown when opening the gallery. Registration still opens the matching tab.","ツールを開いたときに表示するタブです。登録後は、その種類のタブへ移動します。"},
 {"Capture keys","登録キー設定"},{"Keyboard","キーボード"},{"Gamepad","ゲームパッド"},{"Disable","無効化"},{"Reset capture keys","登録キーを初期値に戻す"},
 {"Navigation buttons are reserved. Disable a conflicting capture binding before assigning it.","移動・決定・終了用のボタンは使用できません。登録キー同士を交換する場合は、先に重複する割り当てを無効化してください。"},
 {"Capture key conflicts with another action.","そのキーは別の操作で使用しています。"},{"Capture button is reserved or already assigned.","そのボタンは移動などの操作、または別の登録操作で使用しています。"},
 {"Camera presets","カメラプリセット"},{"Help","使い方"},
 {"1. Wear the items to register, then use a registration button or its assigned key.","希望の装備を身につけて、登録ボタンまたは設定したキーで写真と装備を登録します。"},
 {"Outfit: register worn armor and clothing; weapons and ammunition are excluded.","装備：身につけている防具・衣服を登録します。武器と矢は対象外です。"},
 {"Head: register head equipment only.","頭装備：頭装備のみ登録します。"},
 {"Accessories: wear only the items you want to add, then register.","小物：追加したい装備のみ身につけて登録します。"},
 {"Select a photo below to apply its equipment.","下の写真から希望の装備を選びます。"},
 {"Text tabs: replace the full outfit (organize with tabs).","文字：装備をすべて交換します（タブで分類可能）。"},
 {"Person icon: change only head equipment, keeping your clothes.","人形：着ている装備はそのまま、頭装備のみ変更します。"},
 {"Ring icon: add the pictured items, keeping your clothes; replace items in matching slots.","指輪：着ている装備はそのまま、指定装備を追加します。同じスロットに装備がある場合は交換します。"},
 {"Drag inside the camera frame to move the subject. Use the wheel to change distance.","右の撮影枠をドラッグすると上下左右に位置を調整できます。ホイールで距離を変更します。"},
 {"Right-drag horizontally to orbit; vertically to look up or down.","右ドラッグの左右で回り込み、上下で見上げ・見下ろしを調整できます。"},
 {"Register worn armor and clothing. Applying keeps weapons and ammunition.","装備：防具・衣服一式を登録・交換します。武器と矢はそのまま保持します。"},
 {"Register head equipment. Applying keeps clothes and weapons.","頭装備：頭装備だけを登録。着ている服や武器はそのまま、頭装備のみ変更します。"},
 {"Wear only the accessories to register. Applying adds or replaces their slots.","小物：追加したい装備だけを身につけて登録。選択すると対象の装備を追加し、同じスロットに装備があれば交換します。"},
 {"2. Select a photo below. Text tabs: full outfits. Person icon: head equipment. Ring icon: accessories.","② 下の写真から希望の装備を選びます。文字のタブは装備一式（分類可能）、人型は頭装備、指輪は小物です。"},
 {"D-pad: select. A: apply. LB/RB: tabs. LT: head. RT: accessories. R3: photo menu. B: close.","十字キー：選択／A：着替え／LB・RB：タブ／L2：頭装備／R2：小物／右スティック押し込み：写真メニュー／B：閉じる"},
 {"Enable Add missing base items to restore unowned items. Conflicting worn armor is removed as a whole, including wigs or clothing using multiple slots.","未所持の装備は「不足している装備を追加」で復元できます。スロットが重なる装備は、複数スロットを使うウィッグや服も含め、装備ごと外して交換します。"},
 {"Save accessories","小物を登録"},{"Accessories","小物"},{"[Accessories]","［小物］"},
 {"Wear only the accessories to register. Save photo: L3.","登録したい小物だけを身につけて撮影：左スティック押し込み（L3）"},
 {"No equipped accessories to save.","保存できる小物を装備していません。"},
 {"Accessory slots changed. Register this set again.","小物の使用スロットが変更されています。登録し直してください。"},
 {"Save outfit","装備を登録"},{"Save headgear","頭装備を登録"},
 {"Save outfit and photo: F9 / Y","装備と写真を保存：F9 / Y"},
 {"Save head equipment and photo: gamepad X","頭装備と写真を保存：ゲームパッド X"},
 {"Head equipment slots","頭装備モードの対象スロット"},{"Head equipment","頭装備"},{"Legacy parts","旧・部分装備"},
 {"Save head + photo (X)","頭装備と写真を保存（X）"},
 {"Head capture uses these slots. Saved photos keep their own scope.","30：頭／31：髪／41：長髪／42：サークレット。頭装備撮影の対象です。保存済み写真の対象範囲は変わりません。"},
 {"Save selected slots only","指定スロットのみ保存"},{"Save selected slots + photo","部分装備と写真を保存"},
 {"Head equipment (30, 31, 41, 42)","頭装備（30・31・41・42）"},
 {"Only selected armor slots change. Weapons stay equipped. Select all slots used by a multi-slot item.","指定した装備スロットだけ着替えます。武器は維持します。複数スロット装備は使用スロットをすべて選択してください。"},
 {"Select at least one slot.","保存対象のスロットを選択してください。"},
 {"Columns: %d (click to cycle)","現在%d列・クリックで切り替え"},
 {"Selected slots:","対象スロット："},{"Full outfit","装備一式"},{"[Selected slots]","［部分装備］"},
 {"Confirm permanent deletion","完全削除の確認"},
 {"Empty Trash","ごみ箱を空にする"},{"Delete permanently","完全に削除"},{"Permanently delete these presets and photos? This cannot be undone.","装備プリセットと写真を完全に削除します。元に戻せません。"},{"Cycle columns: 2 / 3 / 4 / 5","列数を切り替え：2 → 3 → 4 → 5"},
 {"Horizontal position","左右位置"},{"Vertical position","上下位置"},{"Look up / down","見上げ（−）／見下ろし（＋）"},{"Fine angle adjustment","角度の微調整"},
 {"Default camera (fixed)","1 は初期構図です（上書き不可）"},{"Load previous custom slot 1","以前 1 に登録した構図を読み込む"},
 {"Custom","カスタム"},{"Adjust composition and press F9/Y to save.","構図を調整してF9/Yで保存"},
 {"Camera preset slot","カメラの登録先"},{"Save current camera to selected slot","現在の構図を選択番号に保存"},
 {"Load camera preset %d","カメラプリセット %d を読み込み"},{"Camera preset %d is empty","カメラプリセット %d は未登録"},
 {"Allow opening from free camera (experimental)","フリーカメラ中の起動を許可（試用）"},
 {"Use a normal first/third-person view, or enable free-camera entry.","通常視点で開くか、オプションでフリーカメラ中の起動を許可してください。"},
 {"Options","オプション"},{"Preset name","登録名"},{"Save outfit + photo","装備と写真を保存"},{"Close studio","閉じる"},
 {"Show preset names","プリセット名を表示"},{"Show item counts","アイテム数を表示"},{"Columns","列数"},{"+ Tab","＋ 分類タブ"},{"Edit tab","タブを編集"},
 {"Camera settings","カメラ設定"},{"Distance","距離"},{"Height","高さ"},{"Orbit","回り込み"},{"Pitch","上下角度"},{"Field of view","視野角"},
 {"Camera and display settings save automatically.","カメラ・表示設定は自動保存されます。"},
 {"Add missing base items (new game)","不足している装備を追加して復元（ニューゲーム用）"},
 {"Base items only. Upgrades, custom enchantments and spells are not saved.","基本アイテムを保存します。強化値・自作付呪・魔法は保存しません。"},
 {"Missing items will be added.","不足する装備の追加：有効"},{"Search outfits","装備を検索"},{"All","すべて"},{"Trash","ごみ箱"},
 {"Favorites","お気に入り"},{"Everyday","普段着"},{"Battle","戦闘用"},{"Dress","ドレス"},
 {"Disabled","無効"},{"D-pad Up","十字キー 上"},{"D-pad Down","十字キー 下"},{"D-pad Left","十字キー 左"},{"D-pad Right","十字キー 右"},{"Left stick click","左スティック押し込み"},{"Right stick click","右スティック押し込み"},
 {"%u items","%u 点"},{"%s  (%u items)","%s（%u 点）"},{"%s (%u items)","%s（%u 点）"},{"%u outfits","%u 件"},
 {"No outfits in this view.","この分類にはプリセットがありません。"},
 {"Up: photos > tabs | Down: tabs > photos | Y: save | A: apply","上：写真→タブ ／ 下：タブ→写真 ／ Y：保存 ／ A：着替え"},
 {"Name","名前"},{"Rename","名前を変更"},{"New tab","タブを追加"},{"Edit current tab","現在のタブを編集"},{"Categories (multiple allowed)","登録する分類（複数選択可）"},
 {"Restore from Trash","ごみ箱から復元"},{"Move to Trash","ごみ箱へ移動"},{"Close","閉じる"},{"Tab name","タブ名"},{"Save tab","タブを保存"},{"Remove tab (keep outfits)","タブを削除（装備は残す）"},{"Cancel","キャンセル"},
 {"Open menu (keyboard): %s","起動キー（キーボード）：%s"},{"Change keyboard key","キーボードの起動キーを変更"},
 {"Open menu (gamepad): %s / hold %.1f s","起動ボタン（パッド）：%s ／ %.1f 秒長押し"},{"Change gamepad button","パッドの起動ボタンを変更"},
 {"Disable gamepad hotkey","パッド起動を無効にする"},{"Hold duration","長押し時間"},{"Cancel binding","キー登録を中止"},
 {"Press the new key/button. Escape or Cancel stops capture.","登録するキー／ボタンを押してください。Escまたは中止で取り消せます。"},
 {"Short gamepad presses pass through. Another mod may also react during a long press; choose an unused button if needed.","短押しは他の機能へ通します。長押しでも他MODが反応する場合は、別のボタンを選んでください。"},
 {"Menu hotkeys saved.","起動キーを保存しました。"},{"Choose a key other than the photo-save key.","撮影キーとは別のキーを選んでください。"},
 {"Save equipment with F9; click a photo to apply a preset.","F9／Yで装備を保存。写真を選ぶと着替えます。"},
 {"Adjust the camera, then save a photo. ESC / F8 closes the studio.","構図を調整してF9／Yで保存。Escまたは起動キーで閉じます。"},
 {"No equipped armor or clothing to save","保存できる装備がありません。"},{"Cannot save a dynamically generated item","動的生成アイテムは保存できません。"},
 {"F9 in the studio saves a photo and equipment. Click a preset photo to apply.","画面内でF9／Yを押すと装備と写真を保存します。写真を選ぶと着替えます。"},
 {"Close this menu and press the configured hotkey (default F8) while standing in a safe open area.","この設定画面を閉じ、通常画面で起動キーを押してください（初期値F8）。"},
 {"Details: Documents/My Games/Skyrim Special Edition/SKSE/OutfitGallery.log","詳細ログ：Documents/My Games/Skyrim Special Edition/SKSE/OutfitGallery.log"}
};
const char* Tr(const char* key) {
    if(language==1) {auto it=japanese.find(key); if(it!=japanese.end()) return it->second.c_str();}
    return key;
}
const char* Label(const char* key) {
    static std::map<std::string,std::string> cache;
    const auto id=std::to_string(language)+key;
    auto [it,inserted]=cache.try_emplace(id);
    if(inserted) it->second=std::string(Tr(key))+"###"+key;
    return it->second.c_str();
}
std::string CategoryTitle(const Category& c) {
    if(c.id=="#head" || c.id=="#legacy" || c.id=="#accessories") return Tr(c.name.c_str());
    if(c.id=="dress" && (c.name=="Dress" || c.name=="Custom")) return Tr("Custom");
    if(c.id.empty() || c.id=="#trash" || (c.id=="favorites"&&c.name=="Favorites") || (c.id=="everyday"&&c.name=="Everyday") || (c.id=="battle"&&c.name=="Battle") || (c.id=="dress"&&c.name=="Dress")) return Tr(c.name.c_str());
    return c.name;
}
std::string StatusText(const std::string& text) {
    if(language!=1) return text;
    if(japanese.contains(text)) return japanese.at(text);
    if(text.starts_with("Cannot open studio: ")) return std::string("画面を開けません：")+Tr(text.substr(20).c_str());
    const std::pair<std::string,std::string> prefixes[]{
        {"Item uses unselected slots: ","未選択のスロットも使う装備があります。対象スロットを確認してください："},
        {"Camera preset saved: ","カメラプリセットを保存しました："},{"Empty camera slot: ","未登録の番号です。カメラ設定から現在の構図を保存してください："},
        {"Photo + outfit saved: ","写真と装備を保存しました："},{"Outfit restored: ","装備の復元を確認しました："},
        {"Equipment differs after apply: ","装備に未反映の項目があります："},{"Missing or changed mod item: ","元のMODまたは装備が見つかりません："},
        {"Equip requests sent: ","装備の変更を要求しました："},{"Cannot open studio: ","画面を開けません："}
    };
    if(text.starts_with("Not owned: ")) {
        auto item=text.substr(11); const auto end=item.find(". Enable Add missing"); if(end!=std::string::npos) item.resize(end);
        return "所持していない装備："+item+"。必要ならオプションの「不足している装備を追加して復元」を有効にしてください。";
    }
    for(const auto& [prefix,translated]:prefixes) if(text.starts_with(prefix)) return translated+text.substr(prefix.size());
    return text; // Low-level diagnostics retain their original detail in the log/UI.
}




