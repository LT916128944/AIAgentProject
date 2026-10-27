# 历史解决方案

触发：报错、不生效、写不成任务、失败、查不到、结果和说的不一致。

1. `search_ue_project(query=现象原文, top_k=1, where={"doc_kind":"history"})`。不读历史/排障 Skill 或对应长文全文
2. 返回的 `title` 与现象对不上，再查一次 `where={"doc_kind":"debug"}`，仍是 `top_k=1`
3. 仍对不上，就当没有现成方案，走域技能，且只读用到的那一节

不触发：纯问答、新功能、没有失败现象。
