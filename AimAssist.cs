using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

[AddComponentMenu("Game/Aim Assist")]
public class AimAssist : MonoBehaviour
{
    [Header("Detection")]
    [Range(1f, 60f)]
    public float detectionAngle = 15f;

    public float detectionRange = 50f;
    public string enemyTag = "Enemy";
    public List<string> headBoneNames = new List<string> { "Head", "head", "Bip001_Head", "mixamorig:Head" };
    public LayerMask obstacleMask = ~0;

    [Header("Snap Settings")]
    [Range(0.01f, 1f)]
    public float snapSpeed = 0.25f;
    public bool rotatePlayerBody = true;

    [Header("Input")]
    public KeyCode aimKey = KeyCode.Mouse1;

    [Header("UI Feedback  (tuỳ chọn)")]
    public Image crosshairImage;
    public Color crosshairNormal = Color.white;
    public Color crosshairLocked = Color.red;

    private Camera _cam;
    private Transform _lockedHead;
    private bool _isAiming;

    void Awake()
    {
        _cam = GetComponentInChildren<Camera>();
        if (_cam == null) _cam = Camera.main;
        if (_cam == null)
            Debug.LogError("[AimAssist] Không tìm thấy Camera.");
    }

    void Update()
    {
        _isAiming = Input.GetKey(aimKey);

        if (_isAiming)
        {
            _lockedHead = FindBestTarget();
            if (_lockedHead != null)
                ApplySnap();
        }
        else
        {
            _lockedHead = null;
        }

        UpdateCrosshairUI();
    }

    Transform FindBestTarget()
    {
        GameObject[] enemies = GameObject.FindGameObjectsWithTag(enemyTag);

        Transform bestHead = null;
        float bestAngle = detectionAngle;

        foreach (var enemy in enemies)
        {
            if (enemy == null) continue;

            Transform head = FindHeadBone(enemy.transform);
            Vector3 targetPos = head != null ? head.position : GetEnemyTopPosition(enemy);
            Vector3 toTarget = targetPos - _cam.transform.position;

            if (toTarget.magnitude > detectionRange) continue;

            float angle = Vector3.Angle(_cam.transform.forward, toTarget.normalized);
            if (angle >= bestAngle) continue;

            if (!HasLineOfSight(targetPos)) continue;

            bestAngle = angle;
            bestHead = head != null ? head : enemy.transform;
        }

        return bestHead;
    }

    bool HasLineOfSight(Vector3 targetPos)
    {
        Vector3 origin = _cam.transform.position;
        Vector3 direction = targetPos - origin;
        float dist = direction.magnitude;

        return !Physics.Raycast(origin, direction.normalized, dist - 0.1f, obstacleMask);
    }

    void ApplySnap()
    {
        Vector3 toHead = (_lockedHead.position - _cam.transform.position).normalized;
        Quaternion targetRot = Quaternion.LookRotation(toHead);
        _cam.transform.rotation = Quaternion.Slerp(_cam.transform.rotation, targetRot, snapSpeed);

        if (rotatePlayerBody)
        {
            Vector3 bodyFwd = new Vector3(toHead.x, 0f, toHead.z);
            if (bodyFwd.sqrMagnitude > 0.001f)
            {
                transform.rotation = Quaternion.Slerp(
                    transform.rotation,
                    Quaternion.LookRotation(bodyFwd),
                    snapSpeed);
            }
        }
    }

    Transform FindHeadBone(Transform root)
    {
        Transform[] all = root.GetComponentsInChildren<Transform>();

        foreach (string boneName in headBoneNames)
            foreach (Transform t in all)
                if (string.Equals(t.name, boneName, System.StringComparison.OrdinalIgnoreCase))
                    return t;

        foreach (Transform t in all)
            if (t.name.ToLower().Contains("head"))
                return t;

        return null;
    }

    Vector3 GetEnemyTopPosition(GameObject enemy)
    {
        Collider col = enemy.GetComponent<Collider>();
        if (col != null)
            return new Vector3(col.bounds.center.x, col.bounds.max.y - 0.1f, col.bounds.center.z);

        return enemy.transform.position + Vector3.up * 1.7f;
    }

    void UpdateCrosshairUI()
    {
        if (crosshairImage == null) return;

        crosshairImage.color = (_isAiming && _lockedHead != null)
            ? crosshairLocked
            : crosshairNormal;
    }

    void OnDrawGizmosSelected()
    {
        if (_cam == null) return;

        Gizmos.color = new Color(0f, 1f, 1f, 0.3f);
        Gizmos.DrawWireSphere(_cam.transform.position, detectionRange);

        float halfRad = detectionAngle * Mathf.Deg2Rad;
        Vector3 fwd = _cam != null ? _cam.transform.forward : Vector3.forward;
        Vector3 right = _cam != null ? _cam.transform.right : Vector3.right;

        Gizmos.color = Color.yellow;
        for (int i = 0; i < 8; i++)
        {
            float a = i * Mathf.PI * 2f / 8f;
            Vector3 dir = Quaternion.AngleAxis(detectionAngle, fwd) *
                          Quaternion.AngleAxis(a * Mathf.Rad2Deg, fwd) * right;
            Gizmos.DrawRay(_cam.transform.position, dir.normalized * detectionRange);
        }

        if (_lockedHead != null)
        {
            Gizmos.color = Color.red;
            Gizmos.DrawLine(_cam.transform.position, _lockedHead.position);
            Gizmos.DrawSphere(_lockedHead.position, 0.1f);
        }
    }
}
