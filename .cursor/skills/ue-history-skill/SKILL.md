---
name: ue-history-skill
description: >-
  排障先查一条历史解决方案，不要读本文件全文。关键词：写不成任务、不生效、报错、失败、查不到、没反应、kind=none、监督者、对不上、不猜。
  用 search_ue_project(现象原文, top_k=1, where={"doc_kind":"history"})。标题对不上再查 doc_kind=debug。仍对不上就走域技能。
---

# 历史解决方案

不要把本文件或 `documents/历史解决方案.md` 整篇读进上下文。现象原文只检索一节。对上标题就按那一节改。对不上就停用本技能。

新现象写进 `documents/历史解决方案.md` 的一个二级标题，再跑 `export_docs.py`。本文件不要追加案例。
