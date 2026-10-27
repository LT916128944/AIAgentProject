# 会话开工协议

1. 开工先跑 `python agent/resume_context.py`（工作目录 `RAG/ue-rag`，用该目录的 `.venv`）
2. 再读 `RAG/ue-rag/data/harness/progress.json` 和 `feature_list.json` 里第一条 `pending`
3. 不读 `WORKFLOW.md` 全文，不读蒸馏目录全文，不读 `ue_mcp_index.json`
4. 模型文本里的「已完成」不改 `feature_list.json` 的 `passed`
5. 描述任务完成时引用 `id` 和 `evidence`
